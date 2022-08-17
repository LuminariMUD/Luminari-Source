/**************************************************************************
 *  File: spell_parser.c                               Part of LuminariMUD *
 *  Usage: Top-level magic routines; outside points of entry to magic sys. *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#define __SPELL_PARSER_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "dg_scripts.h"
#include "fight.h" /* for hit() */
#include "constants.h"
#include "mud_event.h"
#include "spec_procs.h"
#include "class.h"
#include "actions.h"
#include "assign_wpn_armor.h"
#include "domains_schools.h"
#include "grapple.h"
#include "spell_prep.h"
#include "alchemy.h"
#include "missions.h"
#include "psionics.h"
#include "act.h"

#define SINFO spell_info[spellnum]

/* Global Variables definitions, used elsewhere */
struct spell_info_type spell_info[TOP_SKILL_DEFINE];
struct spell_info_type skill_info[TOP_SKILL_DEFINE];
char cast_arg2[MAX_INPUT_LENGTH] = {'\0'};
const char *unused_spellname = "!UNUSED!";       /* So we can get &unused_spellname */
const char *unused_skillname = "!UNUSED!";       /* So we can get &unused_skillname */
const char *unused_wearoff = "!UNUSED WEAROFF!"; /* So we can get &unused_wearoff */

/* Local (File Scope) Function Prototypes */
static void say_spell(struct char_data *ch, int spellnum, struct char_data *tch,
                      struct obj_data *tobj, bool start);
void spello(int spl, const char *name, int max_psp, int min_psp,
            int psp_change, int minpos, int targets, int violent, int routines,
            const char *wearoff, int time, int memtime, int school, bool quest);
void skillo_full(int spl, const char *name, int max_psp, int min_psp,
                 int psp_change, int minpos, int targets, int violent, int routines,
                 const char *wearoff, int time, int memtime, int school, bool quest);
// static int mag_pspcost(struct char_data *ch, int spellnum);

/* Local (File Scope) Variables */
struct syllable
{
  const char *org;
  const char *news;
};
static struct syllable syls[] = {
    {" ", " "},
    {"ar", "abra"},
    {"ate", "i"},
    {"cau", "kada"},
    {"blind", "nose"},
    {"bur", "mosa"},
    {"cu", "judi"},
    {"de", "oculo"},
    {"dis", "mar"},
    {"ect", "kamina"},
    {"en", "uns"},
    {"gro", "cra"},
    {"light", "dies"},
    {"lo", "hi"},
    {"magi", "kari"},
    {"mon", "bar"},
    {"mor", "zak"},
    {"move", "sido"},
    {"ness", "lacri"},
    {"ning", "illa"},
    {"per", "duda"},
    {"ra", "gru"},
    {"re", "candus"},
    {"son", "sabru"},
    {"tect", "infra"},
    {"tri", "cula"},
    {"ven", "nofo"},
    {"word of", "inset"},
    {"a", "i"},
    {"b", "v"},
    {"c", "q"},
    {"d", "m"},
    {"e", "o"},
    {"f", "y"},
    {"g", "t"},
    {"h", "p"},
    {"i", "u"},
    {"j", "y"},
    {"k", "t"},
    {"l", "r"},
    {"m", "w"},
    {"n", "b"},
    {"o", "a"},
    {"p", "s"},
    {"q", "d"},
    {"r", "f"},
    {"s", "g"},
    {"t", "h"},
    {"u", "e"},
    {"v", "z"},
    {"w", "x"},
    {"x", "n"},
    {"y", "l"},
    {"z", "k"},
    {"", ""},
    {"1", "echad"},
    {"2", "shtayim"},
    {"3", "shelosh"},
    {"4", "arba"},
    {"5", "chamesh"},
    {"6", "sheish"},
    {"7", "shevah"},
    {"8", "shmoneh"},
    {"9", "teisha"},
    {"0", "efes"}};

/* may use this for mobs to control their casting
static int mag_pspcost(struct char_data *ch, int spellnum)
{

  return (MAX(SINFO.psp_max - (SINFO.psp_change *
              (GET_LEVEL(ch) - SINFO.min_level[(int) GET_CLASS(ch)])),
          SINFO.psp_min) / 2);
}
 */

/* makes a concentration check for casting, returns TRUE if passed, otherwise
   FALSE = failure and spell should be aborted */
bool concentration_check(struct char_data *ch, int spellnum)
{
  /* concentration check */
  int spell_level = spell_info[spellnum].min_level[CASTING_CLASS(ch)];
  int concentration_dc = 0;

  if (IS_NPC(ch))
  {
    spell_level = MIN(GET_LEVEL(ch), 17);
    spell_level = MAX(1, spell_level);
  }
  else if (CASTING_CLASS(ch) == CLASS_CLERIC)
  {
    spell_level = MIN_SPELL_LVL(spellnum, CLASS_CLERIC, GET_1ST_DOMAIN(ch));
    int spell_level2 = MIN_SPELL_LVL(spellnum, CLASS_CLERIC, GET_2ND_DOMAIN(ch));
    spell_level = MIN(spell_level, spell_level2);
  }
  else if (CASTING_CLASS(ch) == CLASS_INQUISITOR)
  {
    spell_level = MIN_SPELL_LVL(spellnum, CLASS_INQUISITOR, GET_1ST_DOMAIN(ch));
  }
  concentration_dc += spell_level;

  if (spellnum > 0 && spellnum < NUM_SPELLS && HAS_FEAT(ch, FEAT_COMBAT_CASTING))
    concentration_dc -= 4;
  if (spellnum >= PSIONIC_POWER_START && spellnum <= PSIONIC_POWER_END && HAS_FEAT(ch, FEAT_COMBAT_MANIFESTATION))
    concentration_dc -= 4;
  if (!is_tanking(ch))
    concentration_dc -= 10;
  if (char_has_mud_event(ch, eTAUNTED))
    concentration_dc += 6;
  if (char_has_mud_event(ch, eINTIMIDATED))
    concentration_dc += 6;
  if (AFF_FLAGGED(ch, AFF_GRAPPLED) || AFF_FLAGGED(ch, AFF_ENTANGLED))
  {
    if (GRAPPLE_ATTACKER(ch))
      concentration_dc +=
          compute_cmb(GRAPPLE_ATTACKER(ch), COMBAT_MANEUVER_TYPE_GRAPPLE);
    else if (GRAPPLE_TARGET(ch))
      concentration_dc +=
          compute_cmb(GRAPPLE_TARGET(ch), COMBAT_MANEUVER_TYPE_GRAPPLE);
  }

  if (CASTING_CLASS(ch) != CLASS_ALCHEMIST)
  {
    if (AFF_FLAGGED(ch, AFF_DEAF) && dice(1, 5) == 1)
    {
      send_to_char(ch, "Your deafness has made you fumble your spell!\r\n");
      act("$n seems to have fumbled his spell for some reason.", TRUE, ch, 0, 0, TO_ROOM);
      resetCastingData(ch);
      return FALSE;
    }
  }

  if (FIGHTING(ch) && !skill_check(ch, ABILITY_CONCENTRATION, concentration_dc) && CASTING_CLASS(ch) != CLASS_ALCHEMIST)
  {
    send_to_char(ch, "You lost your concentration!\r\n");
    act("$n's concentration is lost, and spell is aborted!", TRUE, ch, 0, 0, TO_ROOM);
    resetCastingData(ch);
    return FALSE;
  }
  else
    return TRUE;
}

/* calculates lowest possible level of a spell (spells can be different
 levels for different classes) */
int lowest_spell_level(int spellnum)
{
  int i, lvl = SINFO.min_level[0];

  for (i = 1; i < NUM_CLASSES; i++)
    if (lvl >= SINFO.min_level[i])
      lvl = SINFO.min_level[i];

  return lvl;
}

/* displays substitute text for spells to represent 'magical phrases' */
static void say_spell(struct char_data *ch, int spellnum, struct char_data *tch,
                      struct obj_data *tobj, bool start)
{
  char lbuf[MEDIUM_STRING], buf[MEDIUM_STRING],
      buf1[MEDIUM_STRING], buf2[MEDIUM_STRING]; /* FIXME */
  const char *format;
  struct char_data *i;
  int j, ofs = 0, dc_of_id = 0, attempt = 0;

  dc_of_id = 20; // DC of identifying the spell

  *buf = '\0';
  strlcpy(lbuf, spell_name(spellnum), sizeof(lbuf));

  while (lbuf[ofs])
  {
    for (j = 0; *(syls[j].org); j++)
    {
      if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org)))
      {
        strlcat(buf, syls[j].news, sizeof(buf)); /* strcat: BAD */
        ofs += strlen(syls[j].org);
        break;
      }
    }
    /* i.e., we didn't find a match in syls[] */
    if (!*syls[j].org)
    {
      log("No entry in syllable table for substring of '%s'", lbuf);
      ofs++;
    }
  }

  if (tch != NULL && IN_ROOM(tch) == IN_ROOM(ch))
  {
    if (tch == ch)
    {
      if (!start)
        format = "\tn$n \tccloses $s eyes and utters the words, '\tC%s\tc'.\tn";
      else
        format =
            "\tn$n \tcweaves $s hands in an \tmintricate\tc pattern and begins to chant the words, '\tC%s\tc'.\tn";
    }
    else
    {
      if (!start)
        format = "\tn$n \tcstares at \tn$N\tc and utters the words, '\tC%s\tc'.\tn";
      else
        format =
            "\tn$n \tcweaves $s hands in an \tmintricate\tc pattern and begins to chant the words, '\tC%s\tc' at \tn$N\tc.\tn";
    }
  }
  else if (tobj != NULL &&
           ((IN_ROOM(tobj) == IN_ROOM(ch)) || (tobj->carried_by == ch)))
  {
    if (!start)
      format = "\tn$n \tcstares at $p and utters the words, '\tC%s\tc'.\tn";
    else
      format = "\tn$n \tcstares at $p and begins chanting the words, '\tC%s\tc'.\tn";
  }
  else
  {
    if (!start)
      format = "\tn$n \tcutters the words, '\tC%s\tc'.\tn";
    else
      format = "\tn$n \tcbegins chanting the words, '\tC%s\tc'.\tn";
  }

  snprintf(buf1, sizeof(buf1), format, spell_name(spellnum));
  snprintf(buf2, sizeof(buf2), format, buf);

  for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room)
  {

    /* need to check why we do not just use act() instead of perform_act() -zusuk */
    if (i == ch || i == tch || !i->desc || !AWAKE(i) ||
        PLR_FLAGGED(i, PLR_WRITING) ||
        ROOM_FLAGGED(IN_ROOM(i), ROOM_SOUNDPROOF))
      continue;

    if (!IS_NPC(i))
      attempt = compute_ability(i, ABILITY_SPELLCRAFT) + d20(i);
    else
      attempt = 10 + d20(i);

    if (attempt > dc_of_id)
      perform_act(buf1, ch, tobj, tch, i, TRUE);
    else
      perform_act(buf2, ch, tobj, tch, i, TRUE);
  }

  if (tch != NULL && !IS_NPC(tch))
    attempt = compute_ability(tch, ABILITY_SPELLCRAFT) + d20(tch);
  else
    attempt = 10 + d20(tch);

  if (tch != NULL && tch != ch && IN_ROOM(tch) == IN_ROOM(ch))
  {
    if (!start)
      snprintf(buf1, sizeof(buf1), "\tn$n \tcstares at you and utters the words, '\tC%s\tc'.\tn",
               attempt > dc_of_id ? spell_name(spellnum) : buf);
    else
      snprintf(buf1, sizeof(buf1),
               "\tn$n \tcweaves $s hands in an intricate pattern and begins to chant the words, '\tC%s\tc' at you.\tn",
               attempt > dc_of_id ? spell_name(spellnum) : buf);
    act(buf1, FALSE, ch, NULL, tch, TO_VICT);
  }
}

const char *spell_name(int num)
{
  if (num > 0 && num <= TOP_SPELL_DEFINE)
    return (spell_info[num].name);
  else if (num == -1)
    return ("Not-Used");
  else
    return ("Non-Spell-Effect");
}

/* checks if a spellnum corresponds to an epic spell */
bool isEpicSpell(int spellnum)
{
  switch (spellnum)
  {
  case SPELL_MUMMY_DUST:
  case SPELL_DRAGON_KNIGHT:
  case SPELL_GREATER_RUIN:
  case SPELL_HELLBALL:
  case SPELL_EPIC_MAGE_ARMOR:
  case SPELL_EPIC_WARDING:
    return TRUE;
  }
  return FALSE;
}

/* This function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and <= TOP_SKILL_DEFINE. */
const char *skill_name(int num)
{
  if (skill_info[num].schoolOfMagic > 0)
    return (skill_info[num].name);
  if (num > 0 && num <= TOP_SKILL_DEFINE)
    return (spell_info[num].name);
  else if (num == -1)
    return ("Not-Used");
  else
    return ("Non-Spell-Effect");
}

/* send a string that is theortically the name of a spell/skill, return
   the spell/skill number
 */
int find_skill_num(char *name)
{
  int skindex, ok;
  char *temp, *temp2;
  char first[MEDIUM_STRING], first2[MEDIUM_STRING], tempbuf[MEDIUM_STRING];

  for (skindex = 1; skindex <= TOP_SKILL_DEFINE; skindex++)
  {
    if (is_abbrev(name, spell_info[skindex].name))
      return (skindex);

    ok = TRUE;
    strlcpy(tempbuf, spell_info[skindex].name, sizeof(tempbuf)); /* strlcpy: OK */
    temp = any_one_arg(tempbuf, first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok)
    {
      if (!is_abbrev(first2, first))
        ok = FALSE;
      temp = any_one_arg(temp, first);
      temp2 = any_one_arg(temp2, first2);
    }

    if (ok && !*first2)
      return (skindex);
  }

  return (-1);
}

/* send a string that is theortically the name of an ability, return
   the ability number
 */
int find_ability_num(char *name)
{
  int skindex, ok;
  char *temp, *temp2;
  char first[MEDIUM_STRING], first2[MEDIUM_STRING], tempbuf[MEDIUM_STRING];

  for (skindex = 1; skindex < NUM_ABILITIES; skindex++)
  {
    if (is_abbrev(name, ability_names[skindex]))
      return (skindex);

    ok = TRUE;
    strlcpy(tempbuf, ability_names[skindex], sizeof(tempbuf));
    temp = any_one_arg(tempbuf, first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok)
    {
      if (!is_abbrev(first2, first))
        ok = FALSE;
      temp = any_one_arg(temp, first);
      temp2 = any_one_arg(temp2, first2);
    }

    if (ok && !*first2)
      return (skindex);
  }

  return (-1);
}

/* This function is the very heart of the entire magic system.  All invocations
 * of all types of magic -- objects, spoken and unspoken PC and NPC spells, the
 * works -- all come through this function eventually. This is also the entry
 * point for non-spoken or unrestricted spells. Spellnum 0 is legal but silently
 * ignored here, to make callers simpler. */
int call_magic(struct char_data *caster, struct char_data *cvict,
               struct obj_data *ovict, int spellnum, int metamagic, int level, int casttype)
{
  int savetype = 0, spell_level = 0;
  struct char_data *tmp = NULL;

  if (spellnum < 1 || spellnum > TOP_SPELL_DEFINE)
    return (0);

  if (!cast_wtrigger(caster, cvict, ovict, spellnum))
    return 0;
  if (!cast_otrigger(caster, ovict, spellnum))
    return 0;
  if (!cast_mtrigger(caster, cvict, spellnum))
    return 0;

  if ((casttype != CAST_WEAPON_POISON) && caster && caster->in_room && caster->in_room != NOWHERE && caster->in_room < top_of_world && ROOM_AFFECTED(caster->in_room, RAFF_ANTI_MAGIC))
  {
    send_to_char(caster, "Your magic fizzles out and dies!\r\n");
    act("$n's magic fizzles out and dies...", FALSE, caster, 0, 0, TO_ROOM);
    return (0);
  }

  if ((casttype != CAST_WEAPON_POISON) && cvict && cvict->in_room && cvict->in_room != NOWHERE && cvict->in_room < top_of_world && ROOM_AFFECTED(cvict->in_room, RAFF_ANTI_MAGIC))
  {
    send_to_char(caster, "Your magic fizzles out and dies!\r\n");
    act("$n's magic fizzles out and dies...", FALSE, caster, 0, 0, TO_ROOM);
    return (0);
  }

  if ((casttype != CAST_WEAPON_POISON) && caster && caster->in_room && caster->in_room != NOWHERE && caster->in_room < top_of_world && ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOMAGIC))
  {
    send_to_char(caster, "Your magic fizzles out and dies.\r\n");
    act("$n's magic fizzles out and dies.", FALSE, caster, 0, 0, TO_ROOM);
    return (0);
  }

  if ((casttype != CAST_WEAPON_POISON) && cvict && cvict->in_room && cvict->in_room != NOWHERE && cvict->in_room < top_of_world && ROOM_FLAGGED(IN_ROOM(cvict), ROOM_NOMAGIC))
  {
    send_to_char(caster, "Your magic fizzles out and dies.\r\n");
    act("$n's magic fizzles out and dies.", FALSE, caster, 0, 0, TO_ROOM);
    return (0);
  }

  if ((casttype != CAST_WEAPON_POISON) && caster && caster->in_room && caster->in_room != NOWHERE && caster->in_room < top_of_world && ROOM_FLAGGED(IN_ROOM(caster), ROOM_PEACEFUL) &&
      (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE)))
  {
    send_to_char(caster, "A flash of white light fills the room, dispelling your violent magic!\r\n");
    act("White light from no particular source suddenly fills the room, then vanishes.", FALSE, caster, 0, 0, TO_ROOM);
    return (0);
  }

  if (cvict && (MOB_FLAGGED(cvict, MOB_NOKILL) || !is_mission_mob(caster, cvict)))
  {
    send_to_char(caster, "This mob is protected.\r\n");
    return (0);
  }

  /* armor arcane failure check, these functions can be found in class.c */
  if (!IS_EPIC_SPELL(spellnum) &&
      (casttype != CAST_INNATE) &&
      (casttype != CAST_POTION) &&
      (casttype != CAST_WEAPON_POISON) &&
      (casttype != CAST_WEAPON_SPELL) &&
      (casttype != CAST_FOOD_DRINK) &&
      (casttype != CAST_BOMB) &&
      (casttype != CAST_SCROLL) &&
      (casttype != CAST_STAFF) &&
      (casttype != CAST_WAND) && !IS_NPC(caster))
    switch (CASTING_CLASS(caster))
    {
    case CLASS_BARD:
      /* bards can wear light armor and cast unpenalized (bard spells) */
      if (compute_gear_armor_type(caster) > ARMOR_TYPE_LIGHT ||
          compute_gear_shield_type(caster) > ARMOR_TYPE_SHIELD)
        if (rand_number(1, 100) <= compute_gear_spell_failure(caster))
        {
          send_to_char(caster, "Your armor ends up hampering your spell!\r\n");
          act("$n's spell is hampered by $s armor!", FALSE, caster, 0, 0, TO_ROOM);
          return 0;
        }
      break;
    case CLASS_SORCERER:
    case CLASS_WIZARD:
      if (rand_number(1, 100) <= compute_gear_spell_failure(caster))
      {
        send_to_char(caster, "Your armor ends up hampering your spell!\r\n");
        act("$n's spell is hampered by $s armor!", FALSE, caster, 0, 0, TO_ROOM);
        return 0;
      }
      break;
    }

  // attach event for epic spells, increase skill
  switch (spellnum)
  {
  case SPELL_MUMMY_DUST:
    attach_mud_event(new_mud_event(eMUMMYDUST, caster, NULL), 9 * SECS_PER_MUD_DAY);
    if (!IS_NPC(caster))
      increase_skill(caster, SKILL_MUMMY_DUST);
    break;
  case SPELL_DRAGON_KNIGHT:
    attach_mud_event(new_mud_event(eDRAGONKNIGHT, caster, NULL), 9 * SECS_PER_MUD_DAY);
    if (!IS_NPC(caster))
      increase_skill(caster, SKILL_DRAGON_KNIGHT);
    break;
  case SPELL_GREATER_RUIN:
    attach_mud_event(new_mud_event(eGREATERRUIN, caster, NULL), 9 * SECS_PER_MUD_DAY);
    if (!IS_NPC(caster))
      increase_skill(caster, SKILL_GREATER_RUIN);
    break;
  case SPELL_HELLBALL:
    attach_mud_event(new_mud_event(eHELLBALL, caster, NULL), 9 * SECS_PER_MUD_DAY);
    if (!IS_NPC(caster))
      increase_skill(caster, SKILL_HELLBALL);
    break;
  case SPELL_EPIC_MAGE_ARMOR:
    attach_mud_event(new_mud_event(eEPICMAGEARMOR, caster, NULL), 9 * SECS_PER_MUD_DAY);
    if (!IS_NPC(caster))
      increase_skill(caster, SKILL_EPIC_MAGE_ARMOR);
    break;
  case SPELL_EPIC_WARDING:
    attach_mud_event(new_mud_event(eEPICWARDING, caster, NULL), 9 * SECS_PER_MUD_DAY);
    if (!IS_NPC(caster))
      increase_skill(caster, SKILL_EPIC_WARDING);
    break;
  }

  /* globe of invulernability spell(s)
   * and spell mantles */
  if (cvict)
  {
    int lvl = lowest_spell_level(spellnum);

    /* minor globe */
    /* we're translating level to circle, so 4 = 2nd circle */
    if (AFF_FLAGGED(cvict, AFF_MINOR_GLOBE) && lvl <= 4 &&
        (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE)))
    {
      send_to_char(caster,
                   "A minor globe from your victim repels your spell!\r\n");
      act("$n's magic is repelled by $N's minor globe spell!", FALSE, caster,
          0, cvict, TO_ROOM);
      if (!FIGHTING(caster))
        set_fighting(caster, cvict);
      if (!FIGHTING(cvict))
        set_fighting(cvict, caster);
      return (0);

      /* major globe */
      /* we're translating level to circle so 8 = 4th circle */
    }
    else if (AFF_FLAGGED(cvict, AFF_GLOBE_OF_INVULN) && lvl <= 8 &&
             (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE)))
    {
      send_to_char(caster, "A globe from your victim repels your spell!\r\n");
      act("$n's magic is repelled by $N's globe spell!", FALSE, caster, 0,
          cvict, TO_ROOM);
      if (!FIGHTING(caster))
        set_fighting(caster, cvict);
      if (!FIGHTING(cvict))
        set_fighting(cvict, caster);
      return (0);

      /* here is spell mantles */
    }
    else if (AFF_FLAGGED(cvict, AFF_SPELL_MANTLE) &&
             GET_SPELL_MANTLE(cvict) > 0 && (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE)))
    {
      send_to_char(caster, "A spell mantle from your victim absorbs your spell!\r\n");
      act("$n's magic is absorbed by $N's spell mantle!", FALSE, caster, 0,
          cvict, TO_ROOM);
      GET_SPELL_MANTLE(cvict)
      --;
      if (GET_SPELL_MANTLE(cvict) <= 0)
      {
        affect_from_char(cvict, SPELL_SPELL_MANTLE);
        affect_from_char(cvict, SPELL_GREATER_SPELL_MANTLE);
        send_to_char(cvict, "\tDYour spell mantle has fallen!\tn\r\n");
      }
      if (!FIGHTING(caster))
        set_fighting(caster, cvict);
      if (!FIGHTING(cvict))
        set_fighting(cvict, caster);
      return (0);
    }
  }

  /* we are going to determine the level of this call here based on casttype
the saving throw will be determined later, so we are just defaulting to
SAVING_WILL here...  */
  switch (casttype)
  {
  case CAST_INNATE:
    savetype = SAVING_WILL;
    spell_level = level;
    break;
  case CAST_WEAPON_POISON:
    savetype = SAVING_FORT;
    spell_level = level;
    break;
  case CAST_WEAPON_SPELL:
    savetype = SAVING_WILL;
    spell_level = level;
    break;
  case CAST_STAFF:
    savetype = SAVING_WILL;
    spell_level = level;
    break;
  case CAST_SCROLL:
    savetype = SAVING_WILL;
    spell_level = level;
    break;
  case CAST_POTION:
    savetype = SAVING_WILL;
    spell_level = level;
    break;
  case CAST_WAND:
    savetype = SAVING_WILL;
    spell_level = level;
    break;

    /* default and casting a spell */
  case CAST_SPELL:
    savetype = SAVING_WILL;
    switch (CASTING_CLASS(caster))
    {
    case CLASS_WIZARD:
      spell_level = level;
      break;
    case CLASS_CLERIC:
      spell_level = level;
      break;
    case CLASS_DRUID:
      spell_level = level;
      break;
    case CLASS_SORCERER:
      spell_level = level;
      break;
    case CLASS_PALADIN:
      spell_level = level;
      break;
    case CLASS_BLACKGUARD:
      spell_level = level;
      break;
    case CLASS_RANGER:
      spell_level = level;
      break;
    case CLASS_BARD:
      spell_level = level;
      break;
    case CLASS_ALCHEMIST:
      spell_level = level;
      break;
    case CLASS_PSIONICIST:
      spell_level = level;
      break;
    case CLASS_INQUISITOR:
      spell_level = level;
      break;
    }

  default:
    savetype = SAVING_WILL;
    spell_level = level;
    break;
  }

  /* spell turning */
  if (cvict)
  {
    if (AFF_FLAGGED(cvict, AFF_SPELL_TURNING) && (SINFO.violent ||
                                                  IS_SET(SINFO.routines, MAG_DAMAGE)))
    {
      send_to_char(caster, "Your spell has been turned!\r\n");
      act("$n's magic is turned by $N!", FALSE, caster, 0,
          cvict, TO_ROOM);
      REMOVE_BIT_AR(AFF_FLAGS(cvict), AFF_SPELL_TURNING);
      tmp = cvict;
      cvict = caster;
      caster = tmp;
    }
  }

  /* now we actually process the spell based on the appropate routine */

  /* special routine handling! */
  switch (spellnum)
  {
  case SPELL_POISON:
    if (caster && cvict && affected_by_spell(cvict, SPELL_POISON))
    {
      damage(caster, cvict, dice(2, 4), SPELL_POISON, KNOWS_DISCOVERY(caster, ALC_DISC_CELESTIAL_POISONS) ? DAM_CELESTIAL_POISON : DAM_POISON, FALSE);
      /* we have custom damage message here for this */
      act("$N suffers further from more poison!",
          FALSE, caster, NULL, cvict, TO_CHAR);
      act("You suffer further from more poison!",
          FALSE, caster, NULL, cvict, TO_VICT | TO_SLEEP);
      act("$N suffers further from more poison!",
          FALSE, caster, NULL, cvict, TO_NOTVICT);
      return 1;
    }
    break;
  default:
    break;
  }

  /* the rest of the routine handling follows: */

  if (IS_SET(SINFO.routines, MAG_DAMAGE))
    if (mag_damage(spell_level, caster, cvict, ovict, spellnum, metamagic, savetype, casttype) == -1)
      return (-1); /* Successful and target died, don't cast again. */

  if (IS_SET(SINFO.routines, MAG_LOOPS))
    mag_loops(spell_level, caster, cvict, ovict, spellnum, savetype, casttype, metamagic);

  if (IS_SET(SINFO.routines, MAG_AFFECTS))
    mag_affects(spell_level, caster, cvict, ovict, spellnum, savetype, casttype, metamagic);

  if (IS_SET(SINFO.routines, MAG_UNAFFECTS))
    mag_unaffects(spell_level, caster, cvict, ovict, spellnum, savetype, casttype);

  if (IS_SET(SINFO.routines, MAG_POINTS))
    mag_points(spell_level, caster, cvict, ovict, spellnum, savetype, casttype);

  if (IS_SET(SINFO.routines, MAG_ALTER_OBJS))
    mag_alter_objs(spell_level, caster, ovict, spellnum, savetype, casttype);

  if (IS_SET(SINFO.routines, MAG_GROUPS))
    mag_groups(spell_level, caster, ovict, spellnum, savetype, casttype);

  if (IS_SET(SINFO.routines, MAG_MASSES))
    mag_masses(spell_level, caster, ovict, spellnum, savetype, casttype, metamagic);

  if (IS_SET(SINFO.routines, MAG_AREAS))
    mag_areas(spell_level, caster, ovict, spellnum, metamagic, savetype, casttype);

  if (IS_SET(SINFO.routines, MAG_SUMMONS))
    mag_summons(spell_level, caster, ovict, spellnum, savetype, casttype);

  if (IS_SET(SINFO.routines, MAG_CREATIONS))
    mag_creations(spell_level, caster, cvict, ovict, spellnum, casttype);

  if (IS_SET(SINFO.routines, MAG_ROOM))
    mag_room(spell_level, caster, ovict, spellnum, casttype);

  /* this switch statement sends us to spells.c for the manual spells */
  if (IS_SET(SINFO.routines, MAG_MANUAL))
    switch (spellnum)
    {
    case SPELL_ACID_ARROW:
      MANUAL_SPELL(spell_acid_arrow);
      break;
    case SPELL_BANISH:
      MANUAL_SPELL(spell_banish);
      break;
    case SPELL_CHARM:
      MANUAL_SPELL(spell_charm);
      break;
    case SPELL_CHARM_ANIMAL:
      MANUAL_SPELL(spell_charm_animal);
      break;
    case SPELL_CLAIRVOYANCE:
      MANUAL_SPELL(spell_clairvoyance);
      break;
    case SPELL_CLOUDKILL:
      MANUAL_SPELL(spell_cloudkill);
      break;
    case SPELL_CONTROL_PLANTS:
      MANUAL_SPELL(spell_control_plants);
      break;
    case SPELL_CONTROL_WEATHER:
      MANUAL_SPELL(spell_control_weather);
      break;
    case SPELL_CREATE_WATER:
      MANUAL_SPELL(spell_create_water);
      break;
    case SPELL_CREEPING_DOOM:
      MANUAL_SPELL(spell_creeping_doom);
      break;
    case SPELL_DETECT_POISON:
      MANUAL_SPELL(spell_detect_poison);
      break;
    case SPELL_DISMISSAL:
      MANUAL_SPELL(spell_dismissal);
      break;
    case SPELL_DISPEL_MAGIC:
      MANUAL_SPELL(spell_dispel_magic);
      break;
    case SPELL_DOMINATE_PERSON:
      MANUAL_SPELL(spell_dominate_person);
      break;
    case SPELL_ENCHANT_ITEM:
      MANUAL_SPELL(spell_enchant_item);
      break;
    case SPELL_GREATER_DISPELLING:
      MANUAL_SPELL(spell_greater_dispelling);
      break;
    case SPELL_GROUP_SUMMON:
      MANUAL_SPELL(spell_group_summon);
      break;
    case SPELL_IDENTIFY:
      MANUAL_SPELL(spell_identify);
      break;
    case SPELL_HOLY_JAVELIN:
      MANUAL_SPELL(spell_holy_javelin);
      break;
    case SPELL_SPIRITUAL_WEAPON:
      MANUAL_SPELL(spell_spiritual_weapon);
      break;
    case SPELL_DANCING_WEAPON:
      MANUAL_SPELL(spell_dancing_weapon);
      break;
    case SPELL_IMPLODE:
      MANUAL_SPELL(spell_implode);
      break;
    case SPELL_INCENDIARY_CLOUD:
      MANUAL_SPELL(spell_incendiary_cloud);
      break;
    case SPELL_LOCATE_CREATURE:
      MANUAL_SPELL(spell_locate_creature);
      break;
    case SPELL_LOCATE_OBJECT:
      MANUAL_SPELL(spell_locate_object);
      break;
    case SPELL_AUGURY:
      MANUAL_SPELL(spell_augury);
      break;
    case SPELL_MASS_DOMINATION:
      MANUAL_SPELL(spell_mass_domination);
      break;
    case SPELL_PLANE_SHIFT:
      MANUAL_SPELL(spell_plane_shift);
      break;
    case SPELL_POLYMORPH:
      MANUAL_SPELL(spell_polymorph);
      break;
    case SPELL_PRISMATIC_SPHERE:
      MANUAL_SPELL(spell_prismatic_sphere);
      break;
    case SPELL_REFUGE:
      MANUAL_SPELL(spell_refuge);
      break;
    case SPELL_SALVATION:
      MANUAL_SPELL(spell_salvation);
      break;
    case SPELL_SPELLSTAFF:
      MANUAL_SPELL(spell_spellstaff);
      break;
    case SPELL_STORM_OF_VENGEANCE:
      MANUAL_SPELL(spell_storm_of_vengeance);
      break;
    case SPELL_SUMMON:
      MANUAL_SPELL(spell_summon);
      break;
    case SPELL_TELEPORT:
      MANUAL_SPELL(spell_teleport);
      break;
    case SPELL_SHADOW_JUMP:
      MANUAL_SPELL(spell_shadow_jump);
      break;
    case SPELL_TRANSPORT_VIA_PLANTS:
      MANUAL_SPELL(spell_transport_via_plants);
      break;
    case SPELL_WALL_OF_FORCE:
      MANUAL_SPELL(spell_wall_of_force);
      break;
    case SPELL_WALL_OF_FIRE:
      MANUAL_SPELL(spell_wall_of_fire);
      break;
    case SPELL_WALL_OF_THORNS:
      MANUAL_SPELL(spell_wall_of_thorns);
      break;
    case SPELL_WIZARD_EYE:
      MANUAL_SPELL(spell_wizard_eye);
      break;
    case SPELL_WORD_OF_RECALL:
      MANUAL_SPELL(spell_recall);
      break;
    // psionics
    case PSIONIC_CONCUSSIVE_ONSLAUGHT:
      MANUAL_SPELL(psionic_concussive_onslaught);
      break;
    case PSIONIC_WALL_OF_ECTOPLASM:
      MANUAL_SPELL(psionic_wall_of_ectoplasm);
      break;
    case PSIONIC_PSYCHOPORTATION:
      MANUAL_SPELL(psionic_psychoportation);
      break;
    } /* end manual spells */

  /* finished routine handling, now we have some code to engage */

  /* NOTE:  this requires a victim, so AoE effects have another
similar method added -zusuk */
  if (SINFO.violent && cvict && GET_POS(cvict) == POS_STANDING &&
      !FIGHTING(cvict) && spellnum != SPELL_CHARM && spellnum != SPELL_CHARM_ANIMAL &&
      spellnum != SPELL_DOMINATE_PERSON)
  {
    if (cvict != caster && IN_ROOM(cvict) == IN_ROOM(caster))
    { // funny results from potions/scrolls
      if (GET_POS(caster) > POS_STUNNED && (FIGHTING(caster) == NULL))
        set_fighting(caster, cvict);
      if (GET_POS(cvict) > POS_STUNNED && (FIGHTING(cvict) == NULL))
      {
        set_fighting(cvict, caster);
      }
    }
  }

  // Always set to zero after applying dc_bonus. We do this here so that AoE spells
  // will give the dc bonus to all targets, not just the first, which occurred when it
  // was removed in mag_savingthrow
  GET_DC_BONUS(caster) = 0;

  return (1);
}

/* mag_objectmagic: This is the entry-point for all magic items.  This should
 * only be called by the 'quaff', 'use', 'recite', etc. routines.
 * For reference, object values 0-3:
 * staff  - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * wand   - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * scroll - [0]	level	[1] spell num   [2] spell num	[3] spell num
 * potion - [0] level	[1] spell num   [2] spell num   [3] spell num
 * Staves and wands will default to level 14 if the level is not specified; the
 * DikuMUD format did not specify staff and wand levels in the world files */
void mag_objectmagic(struct char_data *ch, struct obj_data *obj,
                     char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  int i, k;
  struct char_data *tch = NULL, *next_tch;
  struct obj_data *tobj = NULL;
  int potion_level = GET_OBJ_VAL(obj, 0);

  one_argument(argument, arg, sizeof(arg));

  k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tch, &tobj);

  switch (GET_OBJ_TYPE(obj))
  {
  case ITEM_STAFF:
    act("You tap $p three times on the ground.", FALSE, ch, obj, 0, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, 0, TO_ROOM);
    else
      act("$n taps $p three times on the ground.", FALSE, ch, obj, 0, TO_ROOM);

    if (GET_OBJ_VAL(obj, 2) <= 0 && APOTHEOSIS_SLOTS(ch) < 3)
    {
      send_to_char(ch, "It seems powerless.\r\n");
      act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
    }
    else
    {
      if (APOTHEOSIS_SLOTS(ch) >= 3)
      {
        APOTHEOSIS_SLOTS(ch) -= 3;
        send_to_char(ch, "You power the staff with your focused arcane energy!\r\n");
      }
      else
      {
        GET_OBJ_VAL(obj, 2)
        --;
      }
      USE_STANDARD_ACTION(ch);

      /* Level to cast spell at. */
      k = GET_OBJ_VAL(obj, 0) ? GET_OBJ_VAL(obj, 0) : DEFAULT_STAFF_LVL;

      /* Area/mass spells on staves can cause crashes. So we use special cases
       * for those spells spells here. */
      if (HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, 3), MAG_MASSES | MAG_AREAS))
      {
        for (i = 0, tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
          i++;
        while (i-- > 0)
          call_magic(ch, NULL, NULL, GET_OBJ_VAL(obj, 3), 0, k, CAST_STAFF);
      }
      else
      {
        for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
        {
          next_tch = tch->next_in_room;
          if (ch != tch)
            call_magic(ch, tch, NULL, GET_OBJ_VAL(obj, 3), 0, k, CAST_STAFF);
        }
      }
    }
    break;
  case ITEM_WAND:
    if (k == FIND_CHAR_ROOM)
    {
      if (tch == ch)
      {
        act("You point $p at yourself.", FALSE, ch, obj, 0, TO_CHAR);
        act("$n points $p at $mself.", FALSE, ch, obj, 0, TO_ROOM);
      }
      else
      {
        act("You point $p at $N.", FALSE, ch, obj, tch, TO_CHAR);
        if (obj->action_description)
          act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
        else
          act("$n points $p at $N.", TRUE, ch, obj, tch, TO_ROOM);
      }
    }
    else if (tobj != NULL)
    {
      act("You point $p at $P.", FALSE, ch, obj, tobj, TO_CHAR);
      if (obj->action_description)
        act(obj->action_description, FALSE, ch, obj, tobj, TO_ROOM);
      else
        act("$n points $p at $P.", TRUE, ch, obj, tobj, TO_ROOM);
    }
    else if (IS_SET(spell_info[GET_OBJ_VAL(obj, 3)].routines, MAG_AREAS | MAG_MASSES))
    {
      /* Wands with area spells don't need to be pointed. */
      act("You point $p outward.", FALSE, ch, obj, NULL, TO_CHAR);
      act("$n points $p outward.", TRUE, ch, obj, NULL, TO_ROOM);
    }
    else
    {
      act("At what should $p be pointed?", FALSE, ch, obj, NULL, TO_CHAR);
      return;
    }

    if (GET_OBJ_VAL(obj, 2) <= 0 && APOTHEOSIS_SLOTS(ch) < 3)
    {
      send_to_char(ch, "It seems powerless.\r\n");
      act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
      return;
    }
    if (APOTHEOSIS_SLOTS(ch) >= 3)
    {
      APOTHEOSIS_SLOTS(ch) -= 3;
      send_to_char(ch, "You power the wand with your focused arcane energy!\r\n");
    }
    else
    {
      GET_OBJ_VAL(obj, 2)
      --;
    }
    USE_STANDARD_ACTION(ch);

    if (GET_OBJ_VAL(obj, 0))
      call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3), 0,
                 GET_OBJ_VAL(obj, 0), CAST_WAND);
    else
      call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3), 0,
                 DEFAULT_WAND_LVL, CAST_WAND);
    break;
  case ITEM_SCROLL:
    if (*arg)
    {
      if (!k)
      {
        act("There is nothing to here to affect with $p.", FALSE,
            ch, obj, NULL, TO_CHAR);
        return;
      }
    }
    else
      tch = ch;

    /* AOO */
    if (FIGHTING(ch))
      attack_of_opportunity(FIGHTING(ch), ch, 0);

    act("You recite $p which dissolves.", TRUE, ch, obj, 0, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
    else
      act("$n recites $p.", FALSE, ch, obj, NULL, TO_ROOM);

    USE_STANDARD_ACTION(ch);

    for (i = 1; i <= 3; i++)
      if (call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, i), 0,
                     GET_OBJ_VAL(obj, 0), CAST_SCROLL) <= 0)
        break;

    if (obj != NULL)
      extract_obj(obj);
    break;
  case ITEM_POTION:

    tch = ch;

    /*  AOO */
    if (FIGHTING(ch))
      attack_of_opportunity(FIGHTING(ch), ch, 0);

    if (!consume_otrigger(obj, ch, OCMD_QUAFF)) /* check trigger */
      return;

    act("You quaff $p.", FALSE, ch, obj, NULL, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
    else
      act("$n quaffs $p.", TRUE, ch, obj, NULL, TO_ROOM);

    USE_MOVE_ACTION(ch);

    if (KNOWS_DISCOVERY(ch, ALC_DISC_ENHANCE_POTION) &&
        potion_level < CLASS_LEVEL(ch, CLASS_ALCHEMIST))
      potion_level = CLASS_LEVEL(ch, CLASS_ALCHEMIST);

    for (i = 1; i <= 3; i++)
      if (call_magic(ch, ch, NULL, GET_OBJ_VAL(obj, i), 0, potion_level, CAST_POTION) <= 0)
        break;

    if (obj != NULL)
      extract_obj(obj);
    break;
  default:
    log("SYSERR: Unknown object_type %d in mag_objectmagic.",
        GET_OBJ_TYPE(obj));
    break;
  }
}

void resetCastingData(struct char_data *ch)
{
  IS_CASTING(ch) = FALSE;
  CASTING_TIME(ch) = 0;
  CASTING_SPELLNUM(ch) = 0;
  CASTING_TCH(ch) = NULL;
  CASTING_TOBJ(ch) = NULL;
  GET_AUGMENT_PSP(ch) = 0;
  GET_DC_BONUS(ch) = 0; // another redundancy, but doesn't hurt.  Also removed in mag_saving_throws
}

int castingCheckOk(struct char_data *ch)
{
  int spellnum = CASTING_SPELLNUM(ch);

  /* position check */
  if (GET_POS(ch) <= POS_SITTING)
  {
    act("$n is unable to continue $s spell in $s current position!", FALSE, ch, 0, 0,
        TO_ROOM);
    send_to_char(ch, "You are unable to continue your spell in your current position! (spell aborted)\r\n");
    resetCastingData(ch);
    return 0;
  }

  /* target object available (room target) ? */
  if (CASTING_TOBJ(ch) && CASTING_TOBJ(ch)->in_room != ch->in_room &&
      !IS_SET(SINFO.targets, TAR_OBJ_WORLD | TAR_OBJ_INV))
  {
    act("$n is unable to continue $s spell!", FALSE, ch, 0, 0,
        TO_ROOM);
    send_to_char(ch, "You are unable to find the object for your spell! (spell aborted)\r\n");
    resetCastingData(ch);
    return 0;
  }

  /* target character available? can add long ranged spells here like lightning bolt */
  if (CASTING_TCH(ch) && CASTING_TCH(ch)->in_room != ch->in_room &&
      !IS_SET(SINFO.targets, TAR_CHAR_WORLD))
  {
    act("$n is unable to continue $s spell!", FALSE, ch, 0, 0,
        TO_ROOM);
    send_to_char(ch, "You are unable to find the target for your spell! (spell aborted)\r\n");
    resetCastingData(ch);
    return 0;
  }

  if (AFF_FLAGGED(ch, AFF_NAUSEATED))
  {
    send_to_char(ch, "You are too nauseated to continue casting!\r\n");
    act("$n seems to be too nauseated to continue casting!",
        TRUE, ch, 0, 0, TO_ROOM);
    resetCastingData(ch);
    return (0);
  }

  if (AFF_FLAGGED(ch, AFF_DAZED) || AFF_FLAGGED(ch, AFF_STUN) || AFF_FLAGGED(ch, AFF_PARALYZED) ||
      char_has_mud_event(ch, eSTUNNED))
  {
    send_to_char(ch, "You are unable to continue casting!\r\n");
    act("$n seems to be unable to continue casting!",
        TRUE, ch, 0, 0, TO_ROOM);
    resetCastingData(ch);
    return (0);
  }

  /* made it! */
  return 1;
}

/* moment of completion of spell casting */
void finishCasting(struct char_data *ch)
{

  if (CASTING_SPELLNUM(ch) > 0 && CASTING_SPELLNUM(ch) < NUM_SPELLS)
  {
    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_EMPOWERED_MAGIC))
      GET_DC_BONUS(ch) += 2 * HAS_FEAT(ch, FEAT_EMPOWERED_MAGIC);
  }
  else if (CASTING_SPELLNUM(ch) >= PSIONIC_POWER_START && CASTING_SPELLNUM(ch) <= PSIONIC_POWER_END)
  {
    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_EMPOWERED_PSIONICS))
      GET_DC_BONUS(ch) += HAS_FEAT(ch, FEAT_EMPOWERED_PSIONICS);
    if (affected_by_spell(ch, PSIONIC_ABILITY_PSIONIC_FOCUS))
    {
      GET_DC_BONUS(ch) += 1;
      if (HAS_FEAT(ch, FEAT_PSIONIC_ENDOWMENT))
        GET_DC_BONUS(ch) += 3;
    }
  }

  if (GET_CASTING_CLASS(ch) == CLASS_SHADOWDANCER)
  {

    if (CASTING_SPELLNUM(ch) == SPELL_MIRROR_IMAGE)
    {

      start_daily_use_cooldown(ch, FEAT_SHADOW_ILLUSION);
    }
    else if (CASTING_SPELLNUM(ch) == SPELL_SHADOW_JUMP)
    {
      start_daily_use_cooldown(ch, FEAT_SHADOW_JUMP);
    }
    else if (spell_info[CASTING_SPELLNUM(ch)].schoolOfMagic == CONJURATION)
    {
      start_daily_use_cooldown(ch, FEAT_SHADOW_CALL);
    }
    else if (spell_info[CASTING_SPELLNUM(ch)].schoolOfMagic == EVOCATION)
    {
      start_daily_use_cooldown(ch, FEAT_SHADOW_POWER);
    }
  }

  say_spell(ch, CASTING_SPELLNUM(ch), CASTING_TCH(ch), CASTING_TOBJ(ch), FALSE);
  send_to_char(ch, "You %s...", CASTING_CLASS(ch) == CLASS_ALCHEMIST ? "complete the extract" : (CASTING_CLASS(ch) == CLASS_PSIONICIST ? "complete your manifestation" : "complete your spell"));
  call_magic(ch, CASTING_TCH(ch), CASTING_TOBJ(ch), CASTING_SPELLNUM(ch), CASTING_METAMAGIC(ch),
             (CASTING_CLASS(ch) == CLASS_PSIONICIST) ? GET_PSIONIC_LEVEL(ch) : CASTER_LEVEL(ch), CAST_SPELL);

  if (affected_by_spell(ch, PSIONIC_ABILITY_DOUBLE_MANIFESTATION) && CASTING_SPELLNUM(ch) >= PSIONIC_POWER_START && CASTING_SPELLNUM(ch) <= PSIONIC_POWER_END)
  {
    send_to_char(ch, "\tW[DOUBLE MANIFEST!]\tn");
    call_magic(ch, CASTING_TCH(ch), CASTING_TOBJ(ch), CASTING_SPELLNUM(ch), CASTING_METAMAGIC(ch),
               (CASTING_CLASS(ch) == CLASS_PSIONICIST) ? GET_PSIONIC_LEVEL(ch) : CASTER_LEVEL(ch), CAST_SPELL);
    affect_from_char(ch, PSIONIC_ABILITY_DOUBLE_MANIFESTATION);
  }

  resetCastingData(ch);
}

EVENTFUNC(event_casting)
{
  struct char_data *ch;
  struct mud_event_data *pMudEvent;
  int x, time_stopped = FALSE;
  char buf[MAX_INPUT_LENGTH];

  // initialize everything and dummy checks
  if (event_obj == NULL)
    return 0;
  pMudEvent = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)pMudEvent->pStruct;

  /* we need this or npc's don't have casting time */
  // if (!IS_NPC(ch) && !IS_PLAYING(ch->desc)) return 0;

  int spellnum = CASTING_SPELLNUM(ch);

  // is he casting?
  if (!IS_CASTING(ch))
    return 0;

  // this spell time-stoppable?
  if (IS_AFFECTED(ch, AFF_TIME_STOPPED) &&
      !SINFO.violent && !IS_SET(SINFO.routines, MAG_DAMAGE))
  {
    time_stopped = TRUE;
  }

  // still some time left to cast
  if ((CASTING_TIME(ch) > 0) && !time_stopped &&
      (GET_LEVEL(ch) < LVL_STAFF || IS_NPC(ch)))
  {

    // checking positions, targets
    if (!castingCheckOk(ch))
      return 0;
    else
    {

      if (!concentration_check(ch, spellnum))
        return 0;

      // display time left to finish spell
      snprintf(buf, sizeof(buf), "%s: %s%s%s ", CASTING_CLASS(ch) == CLASS_ALCHEMIST ? "Preparing" : (CASTING_CLASS(ch) == CLASS_PSIONICIST ? "Manifesting" : "Casting"),
               (IS_SET(CASTING_METAMAGIC(ch), METAMAGIC_QUICKEN) ? "quickened " : ""),
               (IS_SET(CASTING_METAMAGIC(ch), METAMAGIC_MAXIMIZE) ? "maximized " : ""),
               SINFO.name);
      for (x = CASTING_TIME(ch); x > 0; x--)
        strlcat(buf, "*", sizeof(buf));
      strlcat(buf, "\r\n", sizeof(buf));
      send_to_char(ch, "%s", buf);

      if (spellnum > 0 && spellnum < NUM_SPELLS)
      {
        /* quick chant feat */
        if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_QUICK_CHANT))
        {
          if (rand_number(0, 1))
          {
            CASTING_TIME(ch)
            --;
          }
        }
        /* wizard quick chant feat */
        if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_WIZ_CHANT))
        {
          if (rand_number(0, 2))
          {
            CASTING_TIME(ch)
            --;
          }
        }
      }

      else if (spellnum >= PSIONIC_POWER_START && spellnum <= PSIONIC_POWER_END)
      {
        // add augment casting time adjustment

        // add above
        if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_QUICK_MIND))
        {
          if (rand_number(0, 1))
          {
            CASTING_TIME(ch)
            --;
          }
        }
        if (affected_by_spell(ch, PSIONIC_ABILITY_PSIONIC_FOCUS))
          CASTING_TIME(ch)
        --;
      }

      CASTING_TIME(ch)
      --;

      // chance quick chant bumped us to finish early
      if (CASTING_TIME(ch) <= 0)
      {

        // do all our checks
        if (!castingCheckOk(ch))
          return 0;

        finishCasting(ch); /* we cleared all our casting checks! */
        return 0;
      }
      else
        return (10);
    }

    // spell needs to be completed now (casting time <= 0)
  }
  else
  {

    // do all our checks
    if (!castingCheckOk(ch))
      return 0;
    else
    { /* we cleared all our casting checks! */
      finishCasting(ch);
      return 0;
    }
  }
}

/* cast_spell is used generically to cast any spoken spell, assuming we already
 * have the target char/obj and spell number.  It checks all restrictions,
 * prints the words, etc. Entry point for NPC casts.  Recommended entry point
 * for spells cast by NPCs via specprocs. */
int cast_spell(struct char_data *ch, struct char_data *tch,
               struct obj_data *tobj, int spellnum, int metamagic)
{
  if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    // imms can cast any spell
    return (call_magic(ch, tch, tobj, spellnum, metamagic, GET_LEVEL(ch), CAST_SPELL));
  }

  int position = GET_POS(ch);
  int ch_class = CLASS_WIZARD, clevel = 0;
  int casting_time = 0;

  if (spellnum < 0 || spellnum > TOP_SPELL_DEFINE)
  {
    log("SYSERR: cast_spell trying to call spellnum %d/%d.", spellnum,
        TOP_SPELL_DEFINE);
    return (0);
  }

  /* these are all (most likely) deprecated dummy checks added by zusuk */
  if (ch && IN_ROOM(ch) > top_of_world)
    return 0;
  if (tch && IN_ROOM(tch) > top_of_world)
    return 0;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) && !is_spellnum_psionic(spellnum))
  {
    send_to_char(ch, "You can not even speak a single word!\r\n");
    return 0;
  }

  if (AFF_FLAGGED(ch, AFF_SILENCED) && !is_spellnum_psionic(spellnum))
  {
    send_to_char(ch, "You are unable to make a sound.\r\n");
    act("$n tries to speak, but cannot seem to make a sound.", TRUE, ch, 0, 0, TO_ROOM);
    return 0;
  }

  // epic spell cooldown
  if (char_has_mud_event(ch, eMUMMYDUST) && spellnum == SPELL_MUMMY_DUST)
  {
    send_to_char(ch, "You must wait longer before you can use this spell again.\r\n");
    return 0;
  }
  if (char_has_mud_event(ch, eDRAGONKNIGHT) && spellnum == SPELL_DRAGON_KNIGHT)
  {
    send_to_char(ch, "You must wait longer before you can use this spell again.\r\n");
    return 0;
  }
  if (char_has_mud_event(ch, eGREATERRUIN) && spellnum == SPELL_GREATER_RUIN)
  {
    send_to_char(ch, "You must wait longer before you can use this spell again.\r\n");
    return 0;
  }
  if (char_has_mud_event(ch, eHELLBALL) && spellnum == SPELL_HELLBALL)
  {
    send_to_char(ch, "You must wait longer before you can use this spell again.\r\n");
    return 0;
  }
  if (char_has_mud_event(ch, eEPICMAGEARMOR) && spellnum == SPELL_EPIC_MAGE_ARMOR)
  {
    send_to_char(ch, "You must wait longer before you can use this spell again.\r\n");
    return 0;
  }
  if (char_has_mud_event(ch, eEPICWARDING) && spellnum == SPELL_EPIC_WARDING)
  {
    send_to_char(ch, "You must wait longer before you can use this spell again.\r\n");
    return 0;
  }

  /* position check */
  if (FIGHTING(ch) && GET_POS(ch) > POS_STUNNED)
    position = POS_FIGHTING;
  if (position < SINFO.min_position)
  {
    switch (position)
    {
    case POS_SLEEPING:
      send_to_char(ch, "You dream about great magical powers.\r\n");
      break;
    case POS_RECLINING:
      send_to_char(ch, "You can't do this reclining!\r\n");
      break;
    case POS_RESTING:
      send_to_char(ch, "You cannot concentrate while resting.\r\n");
      break;
    case POS_SITTING:
      send_to_char(ch, "You can't do this sitting!\r\n");
      break;
    case POS_FIGHTING:
      send_to_char(ch, "Impossible!  You can't concentrate enough!\r\n");
      break;
    default:
      send_to_char(ch, "You can't do much of anything like this!\r\n");
      break;
    }
    return (0);
  }

  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == tch) &&
      (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE)))
  {
    send_to_char(ch, "You are afraid you might hurt your master!\r\n");
    return (0);
  }

  if ((tch != ch) && IS_SET(SINFO.targets, TAR_SELF_ONLY))
  {
    send_to_char(ch, "You can only cast this spell upon yourself!\r\n");
    return (0);
  }

  if ((tch == ch) && IS_SET(SINFO.targets, TAR_NOT_SELF))
  {
    send_to_char(ch, "You cannot cast this spell upon yourself!\r\n");
    return (0);
  }

  if (IS_SET(SINFO.routines, MAG_GROUPS) && !GROUP(ch))
  {
    send_to_char(ch, "You can't cast this spell if you're not in a group!\r\n");
    return (0);
  }

  if (AFF_FLAGGED(ch, AFF_NAUSEATED))
  {
    send_to_char(ch, "You are too nauseated to cast!\r\n");
    act("$n seems to be too nauseated to cast!",
        TRUE, ch, 0, 0, TO_ROOM);
    return (0);
  }

  if (IS_SET(SINFO.targets, TAR_IGNORE) && SINFO.violent)
  {
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE))
    {
      send_to_char(ch, "This room is much too narrow to focus that magic in.\r\n");
      return 0;
    }
  }

  if (char_has_mud_event(ch, eCASTING))
  {
    send_to_char(ch, "You are already attempting to cast!\r\n");
    return (0);
  }

  /* establish base casting time for spell */
  casting_time = SINFO.time;

  /* Going to adjust spell queue and establish what class the character
will be using for casting this spell */
  if (!isEpicSpell(spellnum) && !IS_NPC(ch))
  {

    /* staff get to cast for free */
    if (GET_LEVEL(ch) >= LVL_IMMORT)
    {
      ch_class = CLASS_WIZARD;
      clevel = 30;
      CASTING_CLASS(ch) = ch_class;
    }

    else
    {
      if (psionic_powers[spellnum].psp_cost > 0)
      {
        clevel = GET_PSIONIC_LEVEL(ch);
        CASTING_CLASS(ch) = CLASS_PSIONICIST;
      }
      /*
      // zusuk is commenting this out trying to figure out bugs!
      else if (is_domain_spell_of_ch(ch, spellnum))
      {
              clevel = DIVINE_LEVEL(ch);
              CASTING_CLASS(ch) = CLASS_CLERIC;
      }
      */
      else
      {
        /* SPELL PREPARATION HOOK */
        /* NEW SPELL PREP SYSTEM */
        if (GET_CASTING_CLASS(ch) != CLASS_SHADOWDANCER)
        {
          ch_class = spell_prep_gen_extract(ch, spellnum, metamagic);
          if (ch_class == CLASS_UNDEFINED)
          {
            send_to_char(ch, "ERR:  Report BUG770 to an IMM!\r\n");
            log("spell_prep_gen_extract() failed in cast_spell()");
            return 0;
          }

          /* level to cast this particular spell as */
          clevel = CLASS_LEVEL(ch, ch_class);
          CASTING_CLASS(ch) = ch_class;
        }
        else
        {
          clevel = ARCANE_LEVEL(ch) + CLASS_LEVEL(ch, CLASS_SHADOWDANCER);
        }
        /* npc class */
      }
    }
  }
  else if (IS_NPC(ch))
  {
    ch_class = GET_CLASS(ch);

    /* level to cast this particular spell as */
    clevel = GET_LEVEL(ch);
    CASTING_CLASS(ch) = ch_class;
  }

  /* concentration check */
  if (!concentration_check(ch, spellnum))
  {
    return 0;
  }

  /* meta magic! */
  if (!IS_NPC(ch))
  {
    if (IS_SET(metamagic, METAMAGIC_QUICKEN))
    {
      casting_time = 0;
    }
    if ((ch_class == CLASS_SORCERER || ch_class == CLASS_BARD) &&
        IS_SET(metamagic, METAMAGIC_MAXIMIZE) &&
        !IS_SET(metamagic, METAMAGIC_QUICKEN))
    {
      // Sorcerers with Arcane Bloodline
      if (IS_SET(metamagic, METAMAGIC_ARCANE_ADEPT) || HAS_FEAT(ch, FEAT_ARCANE_APOTHEOSIS))
        ;
      else
        casting_time = casting_time * 3 / 2;
    }
  }

  if (!IS_NPC(ch) && HAS_ELDRITCH_SPELL_CRIT(ch))
  {
    HAS_ELDRITCH_SPELL_CRIT(ch) = false;
    casting_time = 0;
  }

  if (spellnum == PSIONIC_MIND_TRAP)
  {
    casting_time = 0;
  }

  if (spellnum >= PSIONIC_POWER_START && spellnum <= PSIONIC_POWER_END)
  {
    casting_time += get_augment_casting_time_adjustment(ch);
  }

  if (spellnum == PSIONIC_ENERGY_ADAPTATION_SPECIFIED || spellnum == PSIONIC_ENERGY_ADAPTATION)
  {
    if (GET_AUGMENT_PSP(ch) >= 4)
      casting_time = 0;
  }

  /* handle spells with no casting time */
  if (casting_time <= 0 && !IS_NPC(ch))
  { /* we disabled this for npc's */
    send_to_char(ch, "%s", CONFIG_OK);
    say_spell(ch, spellnum, tch, tobj, FALSE);

    /* prevents spell spamming */
    USE_MOVE_ACTION(ch);

    return (call_magic(ch, tch, tobj, spellnum, metamagic, CASTER_LEVEL(ch), CAST_SPELL));
  }
  else
  {

    /* npc's minimum */
    if (IS_NPC(ch))
    {
      casting_time++;
      if (casting_time < 2)
        casting_time = 2;
    }

    /* casting time entry point */
    if (CASTING_CLASS(ch) == CLASS_ALCHEMIST)
    {
      send_to_char(ch, "You begin preparing your extract...\r\n");
      act("$n begins preparing an extract.\r\n", FALSE, ch, 0, 0, TO_ROOM);
    }
    else if (CASTING_CLASS(ch) == CLASS_PSIONICIST)
    {
      send_to_char(ch, "You begin to manifest your power...\r\n");
      act("$n begins manifesting a power.\r\n", FALSE, ch, 0, 0, TO_ROOM);
    }
    else
    {
      send_to_char(ch, "You begin casting your spell...\r\n");
      say_spell(ch, spellnum, tch, tobj, TRUE);
    }
    IS_CASTING(ch) = TRUE;
    CASTING_TIME(ch) = casting_time;
    CASTING_TCH(ch) = tch;
    CASTING_TOBJ(ch) = tobj;
    CASTING_SPELLNUM(ch) = spellnum;
    CASTING_METAMAGIC(ch) = metamagic;

    if (IS_NPC(ch))
    {
      NEW_EVENT(eCASTING, ch, NULL, 2 * PASSES_PER_SEC);
    }
    else
    {
      NEW_EVENT(eCASTING, ch, NULL, 1 * PASSES_PER_SEC);
    }

    /* mandatory wait-state for any spell */
    USE_MOVE_ACTION(ch);
  }
  // this return value has to be checked -zusuk
  return (1);
}

ACMD(do_abort)
{
  if (IS_NPC(ch))
    return;

  if (!IS_CASTING(ch))
  {
    send_to_char(ch, "You aren't casting!\r\n");
    return;
  }

  send_to_char(ch, "You abort your spell.\r\n");
  resetCastingData(ch);
}

/* do_gen_cast is the entry point for PC-casted spells.  It parses the arguments,
 * determines the spell number and finds a target, throws the die to see if
 * the spell can be cast, checks for sufficient psp (hah) and subtracts it, and
 * passes control to cast_spell(). */
ACMDU(do_gen_cast)
{
  struct char_data *tch = NULL;
  struct obj_data *tobj = NULL;
  char *spell_arg = NULL, *target_arg = NULL, *metamagic_arg = NULL;
  int number = 0, spellnum = 0, i = 0, target = 0, metamagic = 0;
  struct affected_type af;
  int class_num = CLASS_UNDEFINED;
  int circle = 99, school = 0;

  if (IS_NPC(ch))
    return;

  /* different cast types are divided here */
  switch (subcmd)
  {
  case SCMD_CAST_SPELL:
    break;
  case SCMD_CAST_PSIONIC:
    break;
  case SCMD_CAST_EXTRACT:
    break;
  case SCMD_CAST_SHADOW:
    if (!*argument)
    {
      display_shadowcast_spells(ch);
      return;
    }
    break;
  default:
    break;
  }

  /* Here I needed to change a bit to grab the metamagic keywords.
   * Valid keywords are:
   *
   *   quickened - Speed up casting
   *   maximized - All variable aspects of spell (dam dice, etc) are maximum. */

  /* get: blank, spell name, target name */
  spell_arg = strtok(argument, "'");

  if (spell_arg == NULL)
  {
    send_to_char(ch, "%s what where?\r\n", do_cast_types[subcmd][0]);
    return;
  }
  spell_arg = strtok(NULL, "'");

  if (spell_arg == NULL)
  {
    send_to_char(ch, "%s names must be enclosed in the Powerful %s Symbols: '\r\n",
                 do_cast_types[subcmd][2], do_cast_types[subcmd][3]);
    return;
  }

  target_arg = strtok(NULL, "\0");

  // log("DEBUG: target t = %s", target_arg);
  // log("DEBUG: Argument = %s", argument);

  /* Check for metamagic. */
  if (subcmd != SCMD_CAST_PSIONIC && subcmd != SCMD_CAST_SHADOW)
  {
    for (metamagic_arg = strtok(argument, " "); metamagic_arg && metamagic_arg[0] != '\''; metamagic_arg = strtok(NULL, " "))
    {
      if (is_abbrev(metamagic_arg, "quickened"))
      {
        if (HAS_FEAT(ch, FEAT_QUICKEN_SPELL))
        {
          SET_BIT(metamagic, METAMAGIC_QUICKEN);
        }
        else
        {
          send_to_char(ch, "You don't know how to quicken your %s!\r\n",
                       do_cast_types[subcmd][4]);
          return;
        }
        // log("DEBUG: Quickened metamagic used.");
      }
      else if (is_abbrev(metamagic_arg, "maximized"))
      {
        if (HAS_FEAT(ch, FEAT_MAXIMIZE_SPELL))
        {
          SET_BIT(metamagic, METAMAGIC_MAXIMIZE);
        }
        else
        {
          send_to_char(ch, "You don't know how to maximize your %s!\r\n",
                       do_cast_types[subcmd][4]);
          return;
        }
      }
      else if (is_abbrev(metamagic_arg, "metamagicadept"))
      {
        if (HAS_FEAT(ch, FEAT_METAMAGIC_ADEPT))
        {
          if (!IS_NPC(ch) && (daily_uses_remaining(ch, FEAT_METAMAGIC_ADEPT)) == 0)
          {
            send_to_char(ch, "You must recover before you can use your metamagic adept ability again.\r\n");
            return;
          }
          if (!IS_NPC(ch))
            start_daily_use_cooldown(ch, FEAT_METAMAGIC_ADEPT);
          SET_BIT(metamagic, METAMAGIC_ARCANE_ADEPT);
        }
        else
        {
          send_to_char(ch, "You do not know the secrets of metamagic adepts!\r\n");
          return;
        }
      }
    }
  }

  spellnum = find_skill_num(spell_arg);

  if ((spellnum < 1) || (spellnum > MAX_SPELLS) || !*spell_arg)
  {
    send_to_char(ch, "%s what?!?\r\n", do_cast_types[subcmd][0]);
    return;
  }

  if (IS_AFFECTED(ch, AFF_TFORM) ||
      IS_AFFECTED(ch, AFF_BATTLETIDE) ||
      affected_by_spell(ch, SPELL_BATTLETIDE))
  {
    send_to_char(ch, "%s?  Why do that when you can SMASH!!\r\n",
                 do_cast_types[subcmd][0]);
    return;
  }

  if (IS_AFFECTED(ch, AFF_WILD_SHAPE) && !HAS_FEAT(ch, FEAT_NATURAL_SPELL))
  {
    send_to_char(ch, "%s?  You have no idea how!\r\n",
                 do_cast_types[subcmd][0]);
    return;
  }

  if (subcmd == SCMD_CAST_PSIONIC)
  {
    if (!IS_PSIONIC(ch) && !IS_EPIC_SPELL(spellnum))
    {
      send_to_char(ch, "You are not even a %s!\r\n", do_cast_types[subcmd][5]);
      return;
    }
  }
  else if (subcmd == SCMD_CAST_SHADOW)
  {
    PREREQ_NOT_NPC();

    if (spellnum == SPELL_MIRROR_IMAGE)
    {
      if (!HAS_REAL_FEAT(ch, FEAT_SHADOW_ILLUSION))
      {
        send_to_char(ch, "You do not have the shadow illusion feat necessary to shadowcast this.\r\n");
        return;
      }

      PREREQ_HAS_USES(FEAT_SHADOW_ILLUSION, "You are not yet able to perform a shadow illusion.\r\n");
    }
    else if (spellnum == SPELL_SHADOW_JUMP)
    {
      if (!HAS_REAL_FEAT(ch, FEAT_SHADOW_JUMP))
      {
        send_to_char(ch, "You do not have the shadow jump feat necessary to shadowcast this.\r\n");
        return;
      }

      PREREQ_HAS_USES(FEAT_SHADOW_JUMP, "You are not yet able to perform a shadow jump.\r\n");
    }
    else
    {
      school = spell_info[spellnum].schoolOfMagic;
      if (school == CONJURATION && HAS_REAL_FEAT(ch, FEAT_SHADOW_CALL))
      {
        PREREQ_HAS_USES(FEAT_SHADOW_CALL, "You are not yet able to shadowcast this spell until you recover a shadow call use.\r\n");
        circle = MIN(circle, compute_spells_circle(CLASS_WIZARD, spellnum, 0, 0));
        if (CLASS_LEVEL(ch, CLASS_SHADOWDANCER) == 10)
        {
          if (circle > 6)
          {
            send_to_char(ch, "That spell is too powerful for you to shadowcast.\r\n");
            return;
          }
        }
        else if (circle > 3)
        {
          send_to_char(ch, "That spell is too powerful for you to shadowcast.\r\n");
          return;
        }
      }
      else if (school == EVOCATION && HAS_REAL_FEAT(ch, FEAT_SHADOW_POWER))
      {
        PREREQ_HAS_USES(FEAT_SHADOW_POWER, "You are not yet able to shadowcast this spell until you recover a shadow power use.\r\n");
        circle = MIN(circle, compute_spells_circle(CLASS_WIZARD, spellnum, 0, 0));
        if (CLASS_LEVEL(ch, CLASS_SHADOWDANCER) == 10)
        {
          if (circle > 7)
          {
            send_to_char(ch, "That spell is too powerful for you to shadowcast.\r\n");
            return;
          }
        }
        else if (circle > 4)
        {
          send_to_char(ch, "That spell is too powerful for you to shadowcast.\r\n");
          return;
        }
      }
      else
      {
        send_to_char(ch, "You are not able to shadowcast that spell.\r\n");
        return;
      }
    }
  }
  else
  {
    if (!IS_CASTER(ch))
    {
      send_to_char(ch, "You are not even a %s!\r\n", do_cast_types[subcmd][5]);
      return;
    }
  }

  if (GET_SKILL(ch, spellnum) == 0 && GET_LEVEL(ch) < LVL_IMMORT && subcmd != SCMD_CAST_PSIONIC && subcmd != SCMD_CAST_SHADOW)
  {
    send_to_char(ch, "You are unfamiliar with that %s.\r\n", do_cast_types[subcmd][2]);
    return;
  }

  if (isEpicSpell(spellnum) && metamagic)
  {
    send_to_char(ch, "Are you trying to implode the universe?!  Sorry, no metamagic "
                     "on epic spells currently!\r\n");
    return;
  }

  /* 3.23.18 Ornir : Add better message when you try to cast a spell from a restricted school. */
  /*if (spell_info[spellnum].schoolOfMagic ==
      restricted_school_reference[GET_SPECIALTY_SCHOOL(ch)]) {
send_to_char(ch, "You are unable to cast spells from this school of magic.\r\n");
return;
}*/

  /* this is the block to make sure they meet the min-level reqs */
  if (isEpicSpell(spellnum))
  {
    switch (spellnum)
    {
    case SPELL_MUMMY_DUST:
      if (!HAS_FEAT(ch, FEAT_MUMMY_DUST))
      {
        send_to_char(ch, "You do not have the 'mummy dust' feat!!\r\n");
        return;
      }
      break;
    case SPELL_DRAGON_KNIGHT:
      if (!HAS_FEAT(ch, FEAT_DRAGON_KNIGHT))
      {
        send_to_char(ch, "You do not have the 'dragon knight' feat!!\r\n");
        return;
      }
      break;
    case SPELL_GREATER_RUIN:
      if (!HAS_FEAT(ch, FEAT_GREATER_RUIN))
      {
        send_to_char(ch, "You do not have the 'greater ruin' feat!!\r\n");
        return;
      }
      break;
    case SPELL_HELLBALL:
      if (!HAS_FEAT(ch, FEAT_HELLBALL))
      {
        send_to_char(ch, "You do not have the 'hellball' feat!!\r\n");
        return;
      }
      break;
    case SPELL_EPIC_MAGE_ARMOR:
      if (!HAS_FEAT(ch, FEAT_EPIC_MAGE_ARMOR))
      {
        send_to_char(ch, "You do not have the 'epic mage armor' feat!!\r\n");
        return;
      }
      break;
    case SPELL_EPIC_WARDING:
      if (!HAS_FEAT(ch, FEAT_EPIC_WARDING))
      {
        send_to_char(ch, "You do not have the 'epic warding' feat!!\r\n");
        return;
      }
      break;
    default:
      send_to_char(ch, "You do not have the appropriate feat!!\r\n");
      return;
    }
  }
  else if (GET_LEVEL(ch) < LVL_IMMORT && subcmd != SCMD_CAST_SHADOW &&
           (BONUS_CASTER_LEVEL(ch, CLASS_WIZARD) + CLASS_LEVEL(ch, CLASS_WIZARD) < SINFO.min_level[CLASS_WIZARD] &&
            //          BONUS_CASTER_LEVEL(ch, CLASS_PSY_WARR) + CLASS_LEVEL(ch, CLASS_PSY_WARR) < SINFO.min_level[CLASS_PSY_WARR] &&
            //          BONUS_CASTER_LEVEL(ch, CLASS_SOULKNIFE) + CLASS_LEVEL(ch, CLASS_SOULKNIFE) < SINFO.min_level[CLASS_SOULKNIFE] &&
            //          BONUS_CASTER_LEVEL(ch, CLASS_WILDER) + CLASS_LEVEL(ch, CLASS_WILDER) < SINFO.min_level[CLASS_WILDER] &&
            BONUS_CASTER_LEVEL(ch, CLASS_CLERIC) + CLASS_LEVEL(ch, CLASS_CLERIC) < MIN_SPELL_LVL(spellnum, CLASS_CLERIC, GET_1ST_DOMAIN(ch)) &&
            BONUS_CASTER_LEVEL(ch, CLASS_CLERIC) + CLASS_LEVEL(ch, CLASS_CLERIC) < MIN_SPELL_LVL(spellnum, CLASS_CLERIC, GET_2ND_DOMAIN(ch)) &&
            BONUS_CASTER_LEVEL(ch, CLASS_INQUISITOR) + CLASS_LEVEL(ch, CLASS_INQUISITOR) < MIN_SPELL_LVL(spellnum, CLASS_INQUISITOR, GET_1ST_DOMAIN(ch)) &&
            BONUS_CASTER_LEVEL(ch, CLASS_DRUID) + CLASS_LEVEL(ch, CLASS_DRUID) < SINFO.min_level[CLASS_DRUID] &&
            BONUS_CASTER_LEVEL(ch, CLASS_RANGER) + CLASS_LEVEL(ch, CLASS_RANGER) < SINFO.min_level[CLASS_RANGER] &&
            BONUS_CASTER_LEVEL(ch, CLASS_PALADIN) + CLASS_LEVEL(ch, CLASS_PALADIN) < SINFO.min_level[CLASS_PALADIN] &&
            BONUS_CASTER_LEVEL(ch, CLASS_BLACKGUARD) + CLASS_LEVEL(ch, CLASS_BLACKGUARD) < SINFO.min_level[CLASS_BLACKGUARD] &&
            BONUS_CASTER_LEVEL(ch, CLASS_BARD) + CLASS_LEVEL(ch, CLASS_BARD) < SINFO.min_level[CLASS_BARD] &&
            BONUS_CASTER_LEVEL(ch, CLASS_PSIONICIST) + CLASS_LEVEL(ch, CLASS_PSIONICIST) < SINFO.min_level[CLASS_PSIONICIST] &&
            BONUS_CASTER_LEVEL(ch, CLASS_SORCERER) + CLASS_LEVEL(ch, CLASS_SORCERER) < SINFO.min_level[CLASS_SORCERER] &&
            BONUS_CASTER_LEVEL(ch, CLASS_ALCHEMIST) + CLASS_LEVEL(ch, CLASS_ALCHEMIST) < SINFO.min_level[CLASS_ALCHEMIST]))
  {
    send_to_char(ch, "You do not know that %s!\r\n", do_cast_types[subcmd][2]);
    return;
  }

  if (subcmd == SCMD_CAST_PSIONIC)
  {
    // has the character added the power via study menu?
    if (!is_a_known_spell(ch, CLASS_PSIONICIST, spellnum) && GET_LEVEL(ch) < LVL_IMMORT)
    {
      send_to_char(ch, "You are not proficient in the use of that power.\r\n");
      return;
    }

    // First, see what is the maximum augment usage allowed
    GET_AUGMENT_PSP(ch) = MIN(GET_AUGMENT_PSP(ch), max_augment_psp_allowed(ch, spellnum));

    // then adjust it to the specifications of the level and power used
    GET_AUGMENT_PSP(ch) = adjust_augment_psp_for_spell(ch, spellnum);

    // we mainly separate the next two checks for the different messages to characters
    if (GET_PSP(ch) < psionic_powers[spellnum].psp_cost)
    {
      send_to_char(ch, "You don't have enough psp to manifest that power.\r\n");
      return;
    }
    if (GET_PSP(ch) < (psionic_powers[spellnum].psp_cost + GET_AUGMENT_PSP(ch)))
    {
      send_to_char(ch, "You don't have enough psp to manifest that power at that augmented amount.\r\n");
      return;
    }
    // All is well, deduct the psp and augment psp
    GET_PSP(ch) -= (psionic_powers[spellnum].psp_cost + GET_AUGMENT_PSP(ch));

    // many powers only benefit from certain intervals of augment psp such
    // as damage bonus = augment psp / 2.  So if their augment psp has a remainder
    // after that calculation, we'll just refund the remainder psp
    if (psionic_powers[spellnum].augment_amount > 1)
      GET_PSP(ch) += (GET_AUGMENT_PSP(ch) % psionic_powers[spellnum].augment_amount);
  }
  else
  {
    /* SPELL PREPARATION HOOK */
    if (GET_LEVEL(ch) < LVL_IMMORT && (class_num = spell_prep_gen_check(ch, spellnum, metamagic)) == CLASS_UNDEFINED &&
        !isEpicSpell(spellnum) && subcmd != SCMD_CAST_SHADOW)
    {
      send_to_char(ch, "You are not ready to %s that %s... (help preparation, or the meta-magic modification might be too high)\r\n",
                   do_cast_types[subcmd][1], do_cast_types[subcmd][2]);
      return;
    }
  }

  if (!IS_NPC(ch))
  {
    if (subcmd != SCMD_CAST_SHADOW)
    {
      GET_CASTING_CLASS(ch) = class_num;
    }
    else
    {
      GET_CASTING_CLASS(ch) = CLASS_SHADOWDANCER;
    }
  }

  circle = compute_spells_circle(GET_CASTING_CLASS(ch), spellnum, 0, 0);

  if (!is_domain_spell_of_ch(ch, spellnum))
  {
    switch (GET_CASTING_CLASS(ch))
    {
    case CLASS_WIZARD:
    case CLASS_ALCHEMIST:
    case CLASS_PSIONICIST:
      if ((10 + circle) > GET_INT(ch))
      {
        send_to_char(ch, "You need to have a minimum intelligence of %d to %s a circle %d %s.\r\n",
                     10 + circle, GET_CASTING_CLASS(ch) == CLASS_WIZARD ? "cast" : ((GET_CASTING_CLASS(ch) == CLASS_ALCHEMIST) ? "imbibe" : "manifest"),
                     circle, GET_CASTING_CLASS(ch) == CLASS_WIZARD ? "spell" : ((GET_CASTING_CLASS(ch) == CLASS_ALCHEMIST) ? "extract" : "power"));
        return;
      }
      break;
    case CLASS_CLERIC:
    case CLASS_DRUID:
    case CLASS_RANGER:
    case CLASS_INQUISITOR:
      if ((10 + circle) > GET_WIS(ch))
      {
        send_to_char(ch, "You need to have a minimum wisdom of %d to cast a circle %d spell.\r\n",
                     10 + circle, circle);
        return;
      }
      break;
    case CLASS_BARD:
    case CLASS_SORCERER:
    case CLASS_PALADIN:
    case CLASS_BLACKGUARD:
      if ((10 + circle) > GET_CHA(ch))
      {
        send_to_char(ch, "You need to have a minimum charisma of %d to cast a circle %d spell.\r\n",
                     10 + circle, circle);
        return;
      }
      break;
    }

    /* further restrictions, this needs updating!
     * what we need to do is loop through the class-array to find the min. stat
     * then compare to the classes - spell-level vs stat
     * -zusuk */
    if (CLASS_LEVEL(ch, CLASS_WIZARD) && GET_INT(ch) < 10)
    {
      send_to_char(ch, "You are not smart enough to cast spells...\r\n");
      return;
    }
    if (CLASS_LEVEL(ch, CLASS_SORCERER) && GET_CHA(ch) < 10)
    {
      send_to_char(ch, "You are not charismatic enough to cast spells...\r\n");
      return;
    }
    if (CLASS_LEVEL(ch, CLASS_BARD) && GET_CHA(ch) < 10)
    {
      send_to_char(ch, "You are not charismatic enough to cast spells...\r\n");
      return;
    }
    if (CLASS_LEVEL(ch, CLASS_CLERIC) && GET_WIS(ch) < 10)
    {
      send_to_char(ch, "You are not wise enough to cast spells...\r\n");
      return;
    }
    if (CLASS_LEVEL(ch, CLASS_RANGER) && GET_WIS(ch) < 10)
    {
      send_to_char(ch, "You are not wise enough to cast spells...\r\n");
      return;
    }
    if (CLASS_LEVEL(ch, CLASS_INQUISITOR) && GET_WIS(ch) < 10)
    {
      send_to_char(ch, "You are not wise enough to cast spells...\r\n");
      return;
    }
    if (CLASS_LEVEL(ch, CLASS_PALADIN) && GET_CHA(ch) < 10)
    {
      send_to_char(ch, "You are not charismatic enough to cast spells...\r\n");
      return;
    }
    if (CLASS_LEVEL(ch, CLASS_BLACKGUARD) && GET_CHA(ch) < 10)
    {
      send_to_char(ch, "You are not charismatic enough to cast spells...\r\n");
      return;
    }
    if (CLASS_LEVEL(ch, CLASS_DRUID) && GET_WIS(ch) < 10)
    {
      send_to_char(ch, "You are not wise enough to cast spells...\r\n");
      return;
    }
    if (CLASS_LEVEL(ch, CLASS_PSIONICIST) && GET_INT(ch) < 10)
    {
      send_to_char(ch, "You are not smart enough to manifest psionic powers...\r\n");
      return;
    }
  }

  /* Find the target */
  if (target_arg != NULL)
  {
    char arg[MAX_INPUT_LENGTH];

    strlcpy(arg, target_arg, sizeof(arg));
    one_argument_u(arg, target_arg);
    skip_spaces(&target_arg);

    /* Copy target to global cast_arg2, for use in spells like locate object */
    strlcpy(cast_arg2, target_arg, sizeof(cast_arg2));
  }

  if (IS_SET(SINFO.targets, TAR_IGNORE))
  {
    target = TRUE;
  }
  else if (target_arg != NULL && *target_arg)
  {
    number = get_number(&target_arg);
    if (!target && (IS_SET(SINFO.targets, TAR_CHAR_ROOM)))
    {
      if ((tch = get_char_vis(ch, target_arg, &number, FIND_CHAR_ROOM)) != NULL)
        target = TRUE;
    }
    if (!target && IS_SET(SINFO.targets, TAR_CHAR_WORLD))
      if ((tch = get_char_vis(ch, target_arg, &number, FIND_CHAR_WORLD)) != NULL)
        target = TRUE;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_INV))
      if ((tobj = get_obj_in_list_vis(ch, target_arg, &number, ch->carrying)) != NULL)
        target = TRUE;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_EQUIP))
    {
      for (i = 0; !target && i < NUM_WEARS; i++)
        if (GET_EQ(ch, i) && isname(target_arg, GET_EQ(ch, i)->name))
        {
          tobj = GET_EQ(ch, i);
          target = TRUE;
        }
    }
    if (!target && IS_SET(SINFO.targets, TAR_OBJ_ROOM))
      if ((tobj = get_obj_in_list_vis(ch, target_arg, &number, world[IN_ROOM(ch)].contents)) != NULL)
        target = TRUE;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_WORLD))
      if ((tobj = get_obj_vis(ch, target_arg, &number)) != NULL)
        target = TRUE;
  }
  else if (!target && FIGHTING(ch) && SINFO.violent)
  {
    target = TRUE;
    tch = FIGHTING(ch);
  }
  else
  { /* if target string is empty */

    if (!target && IS_SET(SINFO.targets, TAR_FIGHT_SELF))
      if (FIGHTING(ch) != NULL)
      {
        tch = ch;
        target = TRUE;
      }
    if (!target && IS_SET(SINFO.targets, TAR_FIGHT_VICT))
      if (FIGHTING(ch) != NULL)
      {
        tch = FIGHTING(ch);
        target = TRUE;
      }
    /* if no target specified, and the spell isn't violent, default to self */
    if (!target && IS_SET(SINFO.targets, TAR_CHAR_ROOM) &&
        !SINFO.violent)
    {
      tch = ch;
      target = TRUE;
    }
    if (!target)
    {
      send_to_char(ch, "Upon %s should the %s be %s?\r\n",
                   IS_SET(SINFO.targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP) ? "what" : "who",
                   do_cast_types[subcmd][2], do_cast_types[subcmd][1]);
      return;
    }
  }

  if (target && (tch == ch) && SINFO.violent && (spellnum != SPELL_DISPEL_MAGIC))
  {
    send_to_char(ch, "You shouldn't %s that on yourself -- could be bad for your health!\r\n",
                 do_cast_types[subcmd][1]);
    return;
  }

  if (!target)
  {
    send_to_char(ch, "Cannot find the target of your %s!\r\n", do_cast_types[subcmd][2]);
    if (subcmd == SCMD_CAST_PSIONIC)
    {
      GET_PSP(ch) += psionic_powers[spellnum].psp_cost + GET_AUGMENT_PSP(ch);
      GET_AUGMENT_PSP(ch) = 0;
    }
    return;
  }

  if (ROOM_AFFECTED(ch->in_room, RAFF_ANTI_MAGIC))
  {
    send_to_char(ch, "Your magic fizzles out and dies!\r\n");
    act("$n's magic fizzles out and dies...", FALSE, ch, 0, 0, TO_ROOM);
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))
  {
    if (subcmd == SCMD_CAST_PSIONIC)
    {
      send_to_char(ch, "Your power fizzles out and dies.\r\n");
      act("$n's power fizzles out and dies.", FALSE, ch, 0, 0, TO_ROOM);
      return;
    }
    else
    {
      send_to_char(ch, "Your magic fizzles out and dies.\r\n");
      act("$n's magic fizzles out and dies.", FALSE, ch, 0, 0, TO_ROOM);
      return;
    }
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) &&
      (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE)))
  {
    send_to_char(ch, "A flash of white light fills the room, dispelling your violent %s!\r\n",
                 do_cast_types[subcmd][4]);
    act("White light from no particular source suddenly fills the room, then vanishes.", FALSE, ch, 0, 0, TO_ROOM);
    return;
  }

  if ((tch != ch) && subcmd == SCMD_CAST_EXTRACT && !KNOWS_DISCOVERY(ch, ALC_DISC_INFUSION) && SINFO.violent == FALSE)
  {
    send_to_char(ch, "You can only use extracts upon yourself!\r\n");
    return;
  }

  if (tch && affected_by_spell(tch, PSIONIC_MIND_TRAP) && !is_player_grouped(ch, tch))
  {
    if (psionic_powers[spellnum].power_type == TELEPATHY || is_spell_mind_affecting(spellnum))
    {
      if (IS_PSIONIC(ch))
      {
        GET_PSP(ch) -= dice(1, 6);
        GET_PSP(ch) = MAX(0, GET_PSP(ch));
        act("Your mental attack on $N triggered $S mind trap, and you feel some power lost!", false, ch, 0, tch, TO_CHAR);
        act("$n's mental attack on YOU has drained $m of some power, due to your mind trap affect!", false, ch, 0, tch, TO_VICT);
      }
      else
      {
        if (!mag_savingthrow(tch, ch, SAVING_FORT, 0, CAST_SPELL, GET_PSIONIC_LEVEL(tch), NOSCHOOL))
        {
          new_affect(&af);
          af.spell = SPELL_AFFECT_MIND_TRAP_NAUSEA;
          af.duration = 1;
          SET_BIT_AR(af.bitvector, AFF_NAUSEATED);
          act("Your mental attack on $N has made YOU nauseated!", false, ch, 0, tch, TO_CHAR);
          act("$n's mental attack on YOU has made $m nauseated, due to your mind trap affect!", false, ch, 0, tch, TO_VICT);
          act("$n's mental attack on $N has made $m nauseated!", false, ch, 0, tch, TO_NOTVICT);
        }
      }
    }
  }

  cast_spell(ch, tch, tobj, spellnum, metamagic);
}

void spell_level(int spell, int chclass, int level)
{
  int bad = 0;

  if (spell < 0 || spell > TOP_SKILL_DEFINE)
  {
    log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, TOP_SKILL_DEFINE);
    return;
  }

  if (chclass < 0 || chclass >= NUM_CLASSES)
  {
    log("SYSERR: assigning '%s' to illegal class %d/%d.", spell_name(spell),
        chclass, NUM_CLASSES - 1);
    bad = 1;
  }

  if (level < 1 || level > LVL_IMPL)
  {
    log("SYSERR: assigning '%s' to illegal level %d/%d.", spell_name(spell),
        level, LVL_IMPL);
    bad = 1;
  }

  if (!bad)
    spell_info[spell].min_level[chclass] = level;
}

/* Assign the spells on boot up */
void spello(int spl, const char *name, int max_psp, int min_psp,
            int psp_change, int minpos, int targets, int violent,
            int routines, const char *wearoff, int time, int memtime, int school,
            bool quest)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++)
    spell_info[spl].min_level[i] = LVL_IMMORT;
  for (i = 0; i < NUM_DOMAINS; i++)
    spell_info[spl].domain[i] = LVL_IMMORT;
  spell_info[spl].psp_max = max_psp;
  spell_info[spl].psp_min = min_psp;
  spell_info[spl].psp_change = psp_change;
  spell_info[spl].min_position = minpos;
  spell_info[spl].targets = targets;
  spell_info[spl].violent = violent;
  spell_info[spl].routines = routines;
  spell_info[spl].name = name;
  if (wearoff == 0)
  {
    char buf[MEDIUM_STRING];
    snprintf(buf, sizeof(buf), "Your '%s' effect has expired", name);
    spell_info[spl].wear_off_msg = strdup(buf);
  }
  else
  {
    spell_info[spl].wear_off_msg = wearoff;
  }
  spell_info[spl].time = time;
  spell_info[spl].memtime = memtime;
  spell_info[spl].schoolOfMagic = school;
  spell_info[spl].quest = quest;
}

void skillo_full(int spl, const char *name, int max_psp, int min_psp,
                 int psp_change, int minpos, int targets, int violent,
                 int routines, const char *wearoff, int time, int memtime, int school,
                 bool quest)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++)
    skill_info[spl].min_level[i] = LVL_IMMORT;
  for (i = 0; i < NUM_DOMAINS; i++)
    skill_info[spl].domain[i] = LVL_IMMORT;
  skill_info[spl].psp_max = max_psp;
  skill_info[spl].psp_min = min_psp;
  skill_info[spl].psp_change = psp_change;
  skill_info[spl].min_position = minpos;
  skill_info[spl].targets = targets;
  skill_info[spl].violent = violent;
  skill_info[spl].routines = routines;
  skill_info[spl].name = name;
  if (wearoff == 0)
  {
    char buf[MEDIUM_STRING];
    snprintf(buf, sizeof(buf), "Your '%s' effect has expired", name);
    skill_info[spl].wear_off_msg = strdup(buf);
  }
  else
  {
    skill_info[spl].wear_off_msg = wearoff;
  }
  skill_info[spl].time = time;
  skill_info[spl].memtime = memtime;
  skill_info[spl].schoolOfMagic = school;
  skill_info[spl].quest = quest;
}

/* initializing the spells as unknown for missing info */
void unused_spell(int spl)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++)
    spell_info[spl].min_level[i] = LVL_IMPL + 1;
  for (i = 0; i < NUM_DOMAINS; i++)
    spell_info[spl].domain[i] = LVL_IMPL + 1;
  spell_info[spl].psp_max = 0;
  spell_info[spl].psp_min = 0;
  spell_info[spl].psp_change = 0;
  spell_info[spl].min_position = POS_DEAD;
  spell_info[spl].targets = 0;
  spell_info[spl].violent = 0;
  spell_info[spl].routines = 0;
  spell_info[spl].name = unused_spellname;
  spell_info[spl].wear_off_msg = unused_wearoff;
  spell_info[spl].time = 0;
  spell_info[spl].memtime = 0;
  spell_info[spl].schoolOfMagic = NOSCHOOL; // noschool
  spell_info[spl].quest = FALSE;
}

void unused_skill(int spl)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++)
    skill_info[spl].min_level[i] = LVL_IMPL + 1;
  for (i = 0; i < NUM_DOMAINS; i++)
    skill_info[spl].domain[i] = LVL_IMPL + 1;
  skill_info[spl].psp_max = 0;
  skill_info[spl].psp_min = 0;
  skill_info[spl].psp_change = 0;
  skill_info[spl].min_position = POS_DEAD;
  skill_info[spl].targets = 0;
  skill_info[spl].violent = 0;
  skill_info[spl].routines = 0;
  skill_info[spl].name = unused_skillname;
  skill_info[spl].wear_off_msg = unused_wearoff;
  skill_info[spl].time = 0;
  skill_info[spl].memtime = 0;
  skill_info[spl].schoolOfMagic = NOSCHOOL; // noschool
  skill_info[spl].quest = FALSE;
}

#define skillo(skill, name, category) spello(skill, name, 0, 0, 0, 0, 0, FALSE, 0, NULL, 0, 0, category, FALSE)

/* Arguments for spello calls:
 * spellnum, maxpsp, minpsp, pspchng, minpos, targets, violent?, routines.
 * spellnum:  Number of the spell.  Usually the symbolic name as defined in
 *  spells.h (such as SPELL_HEAL).
 * spellname: The name of the spell.
 * maxpsp :  The maximum psp this spell will take (i.e., the psp it
 *  will take when the player first gets the spell).
 * minpsp :  The minimum psp this spell will take, no matter how high
 *  level the caster is.
 * pspchng:  The change in psp for the spell from level to level.  This
 *  number should be positive, but represents the reduction in psp cost as
 *  the caster's level increases.
 * minpos  :  Minimum position the caster must be in for the spell to work
 *  (usually fighting or standing).
 * targets :  A "list" of the valid targets for the spell, joined with bitwise
 *  OR ('|').
 * violent :  TRUE or FALSE, depending on if this is considered a violent
 *  spell and should not be cast in PEACEFUL rooms or on yourself.  Should be
 *  set on any spell that inflicts damage, is considered aggressive (i.e.
 *  charm, curse), or is otherwise nasty.
 * routines:  A list of magic routines which are associated with this spell
 *  if the spell uses spell templates.  Also joined with bitwise OR ('|').
 * time:  casting time of the spell
 * memtime:  memtime of the spell
 * schoolOfMagic:  if magical spell, which school does it belong?
 *  Note about - schoolOfMagic:  for skills this is used to categorize it
 * quest:  quest spell or not?  TRUE or FALSE
 */
void mag_assign_spells(void)
{
  int i;

  /* Do not change the loop below. */
  for (i = 0; i <= TOP_SKILL_DEFINE; i++)
    unused_spell(i);
  /*
  for (i = START_SKILLS; i <= TOP_SKILL_DEFINE; i++)
    unused_skill(i);
  */
  /* Do not change the loop above. */

  /** putting skills first **/
  /* Declaration of skills - this assigns categories and also will set it up
   * so that immortals can use these skills by default.  The min level to use
   * the skill for other classes is set up in class.c
   * to-date all skills are assigned to all classes, and a separate function
   * in spec_procs.c checks if the player has the pre-reqs for the skill
   * to be obtained fully. */
  skillo(SKILL_BACKSTAB, "backstab", ACTIVE_SKILL); // 401
  skillo(SKILL_BASH, "bash", ACTIVE_SKILL);
  skillo(SKILL_MUMMY_DUST, "es mummy dust", CASTER_SKILL);
  skillo(SKILL_KICK, "kick", ACTIVE_SKILL);
  skillo(SKILL_WEAPON_SPECIALIST, "weapon specialist", PASSIVE_SKILL); // 405
  skillo(SKILL_WHIRLWIND, "weapon whirlwind", ACTIVE_SKILL);
  skillo(SKILL_RESCUE, "rescue", ACTIVE_SKILL);
  skillo(SKILL_DRAGON_KNIGHT, "es dragon knight", CASTER_SKILL);
  skillo(SKILL_LUCK_OF_HEROES, "luck of heroes", PASSIVE_SKILL);
  skillo(SKILL_TRACK, "track", ACTIVE_SKILL); // 410
  skillo(SKILL_QUICK_CHANT, "quick chant", CASTER_SKILL);
  skillo(SKILL_AMBIDEXTERITY, "ambidexterity", PASSIVE_SKILL);
  skillo(SKILL_DIRTY_FIGHTING, "dirty fighting", PASSIVE_SKILL);
  skillo(SKILL_DODGE, "dodge", PASSIVE_SKILL);
  skillo(SKILL_IMPROVED_CRITICAL, "improved critical", PASSIVE_SKILL); // 415
  skillo(SKILL_MOBILITY, "mobility", PASSIVE_SKILL);
  skillo(SKILL_SPRING_ATTACK, "spring attack", PASSIVE_SKILL);
  skillo(SKILL_TOUGHNESS, "toughness", PASSIVE_SKILL);
  skillo(SKILL_TWO_WEAPON_FIGHT, "two weapon fighting", PASSIVE_SKILL);
  skillo(SKILL_FINESSE, "finesse", PASSIVE_SKILL); // 420
  skillo(SKILL_ARMOR_SKIN, "armor skin", PASSIVE_SKILL);
  skillo(SKILL_BLINDING_SPEED, "blinding speed", PASSIVE_SKILL);
  skillo(SKILL_DAMAGE_REDUC_1, "damage reduction", PASSIVE_SKILL);
  skillo(SKILL_DAMAGE_REDUC_2, "greater damage reduction", PASSIVE_SKILL);
  skillo(SKILL_DAMAGE_REDUC_3, "epic damage reduction", PASSIVE_SKILL); // 425
  skillo(SKILL_EPIC_TOUGHNESS, "epic toughness", PASSIVE_SKILL);
  skillo(SKILL_OVERWHELMING_CRIT, "overwhelming critical", PASSIVE_SKILL);
  skillo(SKILL_SELF_CONCEAL_1, "self concealment", PASSIVE_SKILL);
  skillo(SKILL_SELF_CONCEAL_2, "greater concealment", PASSIVE_SKILL);
  skillo(SKILL_SELF_CONCEAL_3, "epic concealment", PASSIVE_SKILL); // 430
  skillo(SKILL_TRIP, "trip", ACTIVE_SKILL);
  skillo(SKILL_IMPROVED_WHIRL, "improved whirlwind", ACTIVE_SKILL);
  skillo(SKILL_CLEAVE, "cleave (inc)", PASSIVE_SKILL);
  skillo(SKILL_GREAT_CLEAVE, "great_cleave (inc)", PASSIVE_SKILL);
  skillo(SKILL_SPELLPENETRATE, "spell penetration", CASTER_SKILL); // 435
  skillo(SKILL_SPELLPENETRATE_2, "greater spell penetrate", CASTER_SKILL);
  skillo(SKILL_PROWESS, "prowess", PASSIVE_SKILL);
  skillo(SKILL_EPIC_PROWESS, "epic prowess", PASSIVE_SKILL);
  skillo(SKILL_EPIC_2_WEAPON, "epic two weapon fighting", PASSIVE_SKILL);
  skillo(SKILL_SPELLPENETRATE_3, "epic spell penetrate", CASTER_SKILL); // 440
  skillo(SKILL_SPELL_RESIST_1, "spell resistance", CASTER_SKILL);
  skillo(SKILL_SPELL_RESIST_2, "improved spell resist", CASTER_SKILL);
  skillo(SKILL_SPELL_RESIST_3, "greater spell resist", CASTER_SKILL);
  skillo(SKILL_SPELL_RESIST_4, "epic spell resist", CASTER_SKILL);
  skillo(SKILL_SPELL_RESIST_5, "supreme spell resist", CASTER_SKILL); // 445
  skillo(SKILL_INITIATIVE, "initiative", PASSIVE_SKILL);
  skillo(SKILL_EPIC_CRIT, "epic critical", PASSIVE_SKILL);
  skillo(SKILL_IMPROVED_BASH, "improved bash", ACTIVE_SKILL);
  skillo(SKILL_IMPROVED_TRIP, "improved trip", ACTIVE_SKILL);
  skillo(SKILL_POWER_ATTACK, "power attack", ACTIVE_SKILL); // 450
  skillo(SKILL_EXPERTISE, "combat expertise", ACTIVE_SKILL);
  skillo(SKILL_GREATER_RUIN, "es greater ruin", CASTER_SKILL);
  skillo(SKILL_HELLBALL, "es hellball", CASTER_SKILL);
  skillo(SKILL_EPIC_MAGE_ARMOR, "es epic mage armor", CASTER_SKILL);
  skillo(SKILL_EPIC_WARDING, "es epic warding", CASTER_SKILL);                // 455
  skillo(SKILL_RAGE, "rage", ACTIVE_SKILL);                                   // 456
  skillo(SKILL_PROF_MINIMAL, "minimal weapon prof", PASSIVE_SKILL);           // 457
  skillo(SKILL_PROF_BASIC, "basic weapon prof", PASSIVE_SKILL);               // 458
  skillo(SKILL_PROF_ADVANCED, "advanced weapon prof", PASSIVE_SKILL);         // 459
  skillo(SKILL_PROF_MASTER, "master weapon prof", PASSIVE_SKILL);             // 460
  skillo(SKILL_PROF_EXOTIC, "exotic weapon prof", PASSIVE_SKILL);             // 461
  skillo(SKILL_PROF_LIGHT_A, "light armor prof", PASSIVE_SKILL);              // 462
  skillo(SKILL_PROF_MEDIUM_A, "medium armor prof", PASSIVE_SKILL);            // 463
  skillo(SKILL_PROF_HEAVY_A, "heavy armor prof", PASSIVE_SKILL);              // 464
  skillo(SKILL_PROF_SHIELDS, "shield prof", PASSIVE_SKILL);                   // 465
  skillo(SKILL_PROF_T_SHIELDS, "tower shield prof", PASSIVE_SKILL);           // 466
  skillo(SKILL_MURMUR, "murmur(inc)", UNCATEGORIZED);                         // 467
  skillo(SKILL_PROPAGANDA, "propaganda(inc)", UNCATEGORIZED);                 // 468
  skillo(SKILL_LOBBY, "lobby(inc)", UNCATEGORIZED);                           // 469
  skillo(SKILL_STUNNING_FIST, "stunning fist", ACTIVE_SKILL);                 // 470
  skillo(SKILL_MINING, "mining", CRAFTING_SKILL);                             // 471
  skillo(SKILL_HUNTING, "hunting", CRAFTING_SKILL);                           // 472
  skillo(SKILL_FORESTING, "foresting", CRAFTING_SKILL);                       // 473
  skillo(SKILL_KNITTING, "knitting", CRAFTING_SKILL);                         // 474
  skillo(SKILL_CHEMISTRY, "chemistry", CRAFTING_SKILL);                       // 475
  skillo(SKILL_ARMOR_SMITHING, "armor smithing", CRAFTING_SKILL);             // 476
  skillo(SKILL_WEAPON_SMITHING, "weapon smithing", CRAFTING_SKILL);           // 477
  skillo(SKILL_JEWELRY_MAKING, "jewelry making", CRAFTING_SKILL);             // 478
  skillo(SKILL_LEATHER_WORKING, "leather working", CRAFTING_SKILL);           // 479
  skillo(SKILL_FAST_CRAFTER, "fast crafter", CRAFTING_SKILL);                 // 480
  skillo(SKILL_BONE_ARMOR, "bone armor(inc)", CRAFTING_SKILL);                // 481
  skillo(SKILL_ELVEN_CRAFTING, "elven crafting(inc)", CRAFTING_SKILL);        // 482
  skillo(SKILL_MASTERWORK_CRAFTING, "masterwork craft(inc)", CRAFTING_SKILL); // 483
  skillo(SKILL_DRACONIC_CRAFTING, "draconic crafting(inc)", CRAFTING_SKILL);  // 484
  skillo(SKILL_DWARVEN_CRAFTING, "dwarven crafting(inc)", CRAFTING_SKILL);    // 485
  skillo(SKILL_LIGHTNING_REFLEXES, "lightning reflexes", PASSIVE_SKILL);      // 486
  skillo(SKILL_GREAT_FORTITUDE, "great fortitude", PASSIVE_SKILL);            // 487
  skillo(SKILL_IRON_WILL, "iron will", PASSIVE_SKILL);                        // 488
  skillo(SKILL_EPIC_REFLEXES, "epic reflexes", PASSIVE_SKILL);                // 489
  skillo(SKILL_EPIC_FORTITUDE, "epic fortitude", PASSIVE_SKILL);              // 490
  skillo(SKILL_EPIC_WILL, "epic will", PASSIVE_SKILL);                        // 491
  skillo(SKILL_SHIELD_SPECIALIST, "shield specialist", PASSIVE_SKILL);        // 492
  skillo(SKILL_USE_MAGIC, "use magic", ACTIVE_SKILL);                         // 493
  skillo(SKILL_EVASION, "evasion", PASSIVE_SKILL);                            // 494
  skillo(SKILL_IMP_EVASION, "improved evasion", PASSIVE_SKILL);               // 495
  skillo(SKILL_CRIP_STRIKE, "crippling strike", PASSIVE_SKILL);               // 496
  skillo(SKILL_SLIPPERY_MIND, "slippery mind", PASSIVE_SKILL);                // 497
  skillo(SKILL_DEFENSE_ROLL, "defensive roll", PASSIVE_SKILL);                // 498
  skillo(SKILL_GRACE, "divine grace", PASSIVE_SKILL);                         // 499
  skillo(SKILL_DIVINE_HEALTH, "divine health", PASSIVE_SKILL);                // 500
  skillo(SKILL_LAY_ON_HANDS, "lay on hands", ACTIVE_SKILL);                   // 501
  skillo(SKILL_COURAGE, "courage", PASSIVE_SKILL);                            // 502
  skillo(SKILL_SMITE_EVIL, "smite evil", ACTIVE_SKILL);                       // 503
  skillo(SKILL_REMOVE_DISEASE, "purify", ACTIVE_SKILL);                       // 504
  skillo(SKILL_RECHARGE, "recharge", CASTER_SKILL);                           // 505
  skillo(SKILL_STEALTHY, "stealthy", PASSIVE_SKILL);                          // 506
  skillo(SKILL_NATURE_STEP, "nature step", PASSIVE_SKILL);                    // 507
  skillo(SKILL_FAVORED_ENEMY, "favored enemy", PASSIVE_SKILL);                // 508
  skillo(SKILL_DUAL_WEAPONS, "dual weapons", PASSIVE_SKILL);                  // 509
  skillo(SKILL_ANIMAL_COMPANION, "animal companion", ACTIVE_SKILL);           // 510
  skillo(SKILL_PALADIN_MOUNT, "paladin mount", ACTIVE_SKILL);                 // 511
  skillo(SKILL_CALL_FAMILIAR, "call familiar", ACTIVE_SKILL);                 // 512
  skillo(SKILL_PERFORM, "perform", ACTIVE_SKILL);                             // 513
  skillo(SKILL_SCRIBE, "scribe", ACTIVE_SKILL);                               // 514
  skillo(SKILL_TURN_UNDEAD, "turn undead", ACTIVE_SKILL);                     // 515
  skillo(SKILL_WILDSHAPE, "wildshape", ACTIVE_SKILL);                         // 516
  skillo(SKILL_SPELLBATTLE, "spellbattle", ACTIVE_SKILL);                     // 517
  skillo(SKILL_HITALL, "hitall", ACTIVE_SKILL);                               // 518
  skillo(SKILL_CHARGE, "charge", ACTIVE_SKILL);                               // 519
  skillo(SKILL_BODYSLAM, "bodyslam", ACTIVE_SKILL);                           // 520
  skillo(SKILL_SPRINGLEAP, "spring leap", ACTIVE_SKILL);                      // 521
  skillo(SKILL_HEADBUTT, "headbutt", ACTIVE_SKILL);                           // 522
  skillo(SKILL_SHIELD_PUNCH, "shield punch", ACTIVE_SKILL);                   // 523
  skillo(SKILL_DIRT_KICK, "dirt kick", ACTIVE_SKILL);                         // 524
  skillo(SKILL_SAP, "sap", ACTIVE_SKILL);                                     // 525
  skillo(SKILL_SHIELD_SLAM, "shield slam", ACTIVE_SKILL);                     // 526
  skillo(SKILL_SHIELD_CHARGE, "shield charge", ACTIVE_SKILL);                 // 527
  skillo(SKILL_QUIVERING_PALM, "quivering palm", ACTIVE_SKILL);               // 528
  skillo(SKILL_SURPRISE_ACCURACY, "surprise accuracy", ACTIVE_SKILL);         // 529
  skillo(SKILL_POWERFUL_BLOW, "powerful blow", ACTIVE_SKILL);                 // 530
  skillo(SKILL_RAGE_FATIGUE, "rage fatigue", ACTIVE_SKILL);                   // 531
  skillo(SKILL_COME_AND_GET_ME, "come and get me", ACTIVE_SKILL);             // 532
  skillo(SKILL_FEINT, "feint", ACTIVE_SKILL);                                 // 533
  skillo(SKILL_SMITE_GOOD, "smite good", ACTIVE_SKILL);                       // 534
  skillo(SKILL_SMITE_DESTRUCTION, "destructive smite", ACTIVE_SKILL);         // 535
  skillo(SKILL_DESTRUCTIVE_AURA, "destructive aura", ACTIVE_SKILL);           // 536
  skillo(SKILL_AURA_OF_PROTECTION, "protection aura", ACTIVE_SKILL);          // 537
  skillo(SKILL_DEATH_ARROW, "arrow of death", ACTIVE_SKILL);                  // 538
  skillo(SKILL_DEFENSIVE_STANCE, "defensive stance", ACTIVE_SKILL);           // 539
  skillo(SKILL_CRIPPLING_CRITICAL, "crippling critical", PASSIVE_SKILL);      // 540
  skillo(SKILL_DRHRT_CLAWS, "draconic heritage claws", ACTIVE_SKILL);         // 541
  skillo(SKILL_DRHRT_WINGS, "draconic heritage wings", ACTIVE_SKILL);         // 542
  skillo(SKILL_BOMB_TOSS, "bomb toss", ACTIVE_SKILL);                         /* 543 */
  skillo(SKILL_MUTAGEN, "mutagen", ACTIVE_SKILL);                             /* 544 */
  skillo(SKILL_COGNATOGEN, "cognatogen", ACTIVE_SKILL);                       /* 545 */
  skillo(SKILL_INSPIRING_COGNATOGEN, "inspiring cognatogen", ACTIVE_SKILL);   /* 546 */
  skillo(SKILL_PSYCHOKINETIC, "psychokinetic", ACTIVE_SKILL);                 /* 547 */
  skillo(SKILL_INNER_FIRE, "inner fire", ACTIVE_SKILL);                       /* 548 */
  skillo(SKILL_SACRED_FLAMES, "sacred flames", ACTIVE_SKILL);                 /* 549 */
  skillo(SKILL_EPIC_WILDSHAPE, "epic wildshape", PASSIVE_SKILL);

  /* songs */
  skillo(SKILL_SONG_OF_FOCUSED_MIND, "song of focused mind", ACTIVE_SKILL);       // 588
  skillo(SKILL_SONG_OF_FEAR, "song of fear", ACTIVE_SKILL);                       // 589
  skillo(SKILL_SONG_OF_ROOTING, "song of rooting", ACTIVE_SKILL);                 // 590
  skillo(SKILL_SONG_OF_THE_MAGI, "song of the magi", ACTIVE_SKILL);               // 591
  skillo(SKILL_SONG_OF_HEALING, "song of healing", ACTIVE_SKILL);                 // 592
  skillo(SKILL_DANCE_OF_PROTECTION, "dance of protection", ACTIVE_SKILL);         // 593
  skillo(SKILL_SONG_OF_FLIGHT, "song of flight", ACTIVE_SKILL);                   // 594
  skillo(SKILL_SONG_OF_HEROISM, "song of heroism", ACTIVE_SKILL);                 // 595
  skillo(SKILL_ORATORY_OF_REJUVENATION, "oratory of rejuvenation", ACTIVE_SKILL); // 596
  skillo(SKILL_ACT_OF_FORGETFULNESS, "skit of forgetfulness", ACTIVE_SKILL);      // 597
  skillo(SKILL_SONG_OF_REVELATION, "song of revelation", ACTIVE_SKILL);           // 598
  skillo(SKILL_SONG_OF_DRAGONS, "song of dragons", ACTIVE_SKILL);                 // 599
                                                                                  /* end songs */

  /****note weapon specialist and luck of heroes inserted in free slots ***/

  /** next assigning the psionic powers from psionics.c */
  assign_psionic_powers();

  // sorted the spells by shared / magical / divine, and by circle
  // in each category (school) -zusuk
  /* please leave these here for my usage -zusuk */
  /* evocation */
  /* conjuration */
  /* necromancy */
  /* enchantment */
  /* illusion */
  /* divination */
  /* abjuration */
  /* transmutation */

  // shared
  spello(SPELL_INFRAVISION, "infravision", 0, 0, 0, POS_FIGHTING, // enchant
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your night vision seems to fade.", 13, 13,
         ENCHANTMENT, FALSE); // wizard 4, cleric 4
  spello(SPELL_DETECT_POISON, "detect poison", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL,
         "The detect poison wears off.", 19, 10,
         DIVINATION, FALSE);                            // wizard 7, cleric 2
  spello(SPELL_POISON, "poison", 0, 0, 0, POS_FIGHTING, // enchantment
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV, TRUE,
         MAG_AFFECTS | MAG_ALTER_OBJS,
         "You feel less sick.", 5, 13,
         ENCHANTMENT, FALSE); // wizard 4, cleric 5
  spello(SPELL_ENERGY_DRAIN, "energy drain", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_MANUAL,
         NULL, 9, 23,
         NECROMANCY, FALSE);                                        // wizard 9, cleric 9
  spello(SPELL_REMOVE_CURSE, "remove curse", 0, 0, 0, POS_FIGHTING, // abjur
         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE,
         MAG_UNAFFECTS | MAG_ALTER_OBJS,
         NULL, 4, 13, ABJURATION, FALSE); // wizard 4, cleric 4
  spello(SPELL_ENDURANCE, "endurance", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your magical endurance has faded away.", 2, 11,
         TRANSMUTATION, FALSE); // wizard 1, cleric 1
  spello(SPELL_RESIST_ENERGY, "resist energy", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your energy resistance dissipates.", 2, 11,
         ABJURATION, FALSE); // wizard 1, cleric 1
  spello(SPELL_CUNNING, "cunning", 30, 15, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your magical cunning has faded away.", 3, 11,
         TRANSMUTATION, FALSE); // wizard 2, cleric 3
  spello(SPELL_WISDOM, "wisdom", 30, 15, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your magical wisdom has faded away.", 3, 11,
         TRANSMUTATION, FALSE); // wizard 2, cleric 2
  spello(SPELL_CHARISMA, "charisma", 30, 15, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your magical charisma has faded away.", 3, 11,
         TRANSMUTATION, FALSE); // wizard 2, cleric 2
  spello(SPELL_CONTROL_WEATHER, "control weather", 72, 57, 1, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_MANUAL,
         NULL, 14, 19, CONJURATION, FALSE); // wiz 7, cleric x
  spello(SPELL_NEGATIVE_ENERGY_RAY, "negative energy ray", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 1, 7, NECROMANCY, FALSE); // wiz 1, cleric 1
  spello(SPELL_ENDURE_ELEMENTS, "endure elements", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel your element protection wear off.", 2, 7, ABJURATION, FALSE); // wiz1 cle1
  spello(SPELL_PROT_FROM_EVIL, "protection from evil", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel less protected from evil.", 5, 7, ABJURATION, FALSE); // wiz1 cle1
  spello(SPELL_PROT_FROM_GOOD, "protection from good", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel less protected from good.", 5, 7, ABJURATION, FALSE); // wiz1 cle1
  spello(SPELL_SUMMON_CREATURE_1, "summon creature i", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS,
         NULL, 5, 10, CONJURATION, FALSE); // wiz1, cle1
  spello(SPELL_STRENGTH, "strength", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel weaker.", 2, 7, TRANSMUTATION, FALSE); // wiz2, cle1
  spello(SPELL_GRACE, "grace", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel less dextrous.", 2, 7, TRANSMUTATION, FALSE); // wiz2, cle1
  spello(SPELL_SCARE, "scare", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You no longer feel scared.", 1, 7, ILLUSION, FALSE); // wiz1, cle2
  spello(SPELL_FEAR, "cause fear", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You no longer feel afraid.", 1, 7, ILLUSION, FALSE); // wiz4, cle5
  spello(SPELL_SUMMON_CREATURE_2, "summon creature ii", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS,
         NULL, 6, 9, CONJURATION, FALSE); // wiz2, cle2
  spello(SPELL_DETECT_MAGIC, "detect magic", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "The detect magic wears off.", 1, 9, DIVINATION, FALSE); // wiz2, cle2
  spello(SPELL_DARKNESS, "darkness", 0, 0, 0, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_ROOM,
         "The cloak of darkness in the area dissolves.", 5, 9, DIVINATION, FALSE); // wiz2, cle2
  spello(SPELL_SUMMON_CREATURE_3, "summon creature iii", 95, 80, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS,
         NULL, 7, 11, CONJURATION, FALSE); // wiz3, cle3
  spello(SPELL_DEAFNESS, "deafness", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You feel like you can hear again.", 3, 11,
         NECROMANCY, FALSE); // wiz2, cle3
  spello(SPELL_DISPEL_MAGIC, "dispel magic", 65, 50, 1, POS_FIGHTING,
         TAR_OBJ_ROOM | TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_MANUAL,
         NULL, 4, 11, DIVINATION, FALSE); // wiz3, cle3
  spello(SPELL_ANIMATE_DEAD, "animate dead", 72, 57, 1, POS_FIGHTING,
         TAR_OBJ_ROOM, FALSE, MAG_SUMMONS,
         NULL, 10, 13, NECROMANCY, FALSE); // wiz4, cle3
  spello(SPELL_SUMMON_CREATURE_4, "summon creature iv", 95, 80, 1,
         POS_FIGHTING, TAR_IGNORE, FALSE, MAG_SUMMONS, NULL, 8, 13, CONJURATION, FALSE); // wiz4, cle4
  spello(SPELL_BLINDNESS, "blindness", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You feel a cloak of blindness dissolve.", 3, 11,
         NECROMANCY, FALSE); // wiz2, cle3
  spello(SPELL_CIRCLE_A_EVIL, "circle against evil", 58, 43, 1,
         POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS,
         NULL, 7, 13, ABJURATION, FALSE); // wiz3 cle4
  spello(SPELL_CIRCLE_A_GOOD, "circle against good", 58, 43, 1,
         POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS,
         NULL, 7, 13, ABJURATION, FALSE); // wiz3 cle4
  spello(SPELL_CURSE, "curse", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OBJ_INV, TRUE, MAG_AFFECTS | MAG_ALTER_OBJS, "You feel more optimistic.", 7, 13, NECROMANCY, FALSE); // wiz4 cle4
  spello(SPELL_DAYLIGHT, "daylight", 50, 25, 5, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_ROOM,
         "The artificial daylight fades away.", 6, 13, ILLUSION, FALSE); // wiz3, cle4
  spello(SPELL_SUMMON_CREATURE_6, "summon creature vi", 0, 0, 0,
         POS_FIGHTING, TAR_IGNORE, FALSE, MAG_SUMMONS, NULL, 9, 17, CONJURATION, FALSE); // wiz6 cle6
  spello(SPELL_EYEBITE, "eyebite", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS | MAG_ALTER_OBJS, "You feel the disease fade away.", 6, 17, NECROMANCY, FALSE); // wiz6 cle6
  spello(SPELL_MASS_WISDOM, "mass wisdom", 0, 0, 0, POS_FIGHTING, TAR_IGNORE,                                                              // wiz7, cle6
         FALSE, MAG_GROUPS, "The wisdom spell fades away.", 5, 19, TRANSMUTATION, FALSE);                                                  // wiz7, cle6
  spello(SPELL_MASS_CHARISMA, "mass charisma", 0, 0, 0, POS_FIGHTING, TAR_IGNORE,                                                          // wiz7, cle6
         FALSE, MAG_GROUPS, "The charisma spell fades away.", 5, 19, TRANSMUTATION, FALSE);
  spello(SPELL_MASS_CUNNING, "mass cunning", 0, 0, 0, POS_FIGHTING, TAR_IGNORE,
         FALSE, MAG_GROUPS, "The cunning spell fades away.", 5, 19, TRANSMUTATION, FALSE);
  spello(SPELL_MASS_STRENGTH, "mass strength", 0, 0, 0, POS_FIGHTING, TAR_IGNORE,
         FALSE, MAG_GROUPS, "You feel weaker.", 5, 19, TRANSMUTATION, FALSE);
  spello(SPELL_MASS_GRACE, "mass grace", 0, 0, 0, POS_FIGHTING, TAR_IGNORE,
         FALSE, MAG_GROUPS, "You feel less dextrous.", 5, 19, TRANSMUTATION, FALSE);
  spello(SPELL_MASS_ENDURANCE, "mass endurance", 0, 0, 0, POS_FIGHTING, TAR_IGNORE,
         FALSE, MAG_GROUPS, "Your magical endurance has faded away.", 5, 19, TRANSMUTATION, FALSE);
  /**  end shared list **/

  // shared epic
  spello(SPELL_DRAGON_KNIGHT, "dragon knight", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS,
         NULL, 12, 1,
         NOSCHOOL, FALSE);
  spello(SPELL_GREATER_RUIN, "greater ruin", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 6, 1,
         NOSCHOOL, FALSE);
  spello(SPELL_HELLBALL, "hellball", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 6, 1,
         NOSCHOOL, FALSE);
  spello(SPELL_MUMMY_DUST, "mummy dust", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS,
         NULL, 14, 1,
         NOSCHOOL, FALSE);

  // paladin
  /* = =  4th circle  = = */
  spello(SPELL_HOLY_SWORD, "holy weapon", 37, 22, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_CREATIONS,
         NULL, 2, 23, NOSCHOOL, FALSE);

  // blackguard
  spello(SPELL_UNHOLY_SWORD, "unholy weapon", 37, 22, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_CREATIONS,
         NULL, 2, 23, NOSCHOOL, FALSE);

  // magical

  /* = =  cantrips  = = */
  /* evocation */
  spello(SPELL_ACID_SPLASH, "acid splash", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 0, 1, EVOCATION, FALSE);
  spello(SPELL_RAY_OF_FROST, "ray of frost", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 0, 1, EVOCATION, FALSE);

  /* = =  1st circle  = = */
  /* evocation */
  spello(SPELL_MAGIC_MISSILE, "magic missile", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_LOOPS,
         NULL, 0, 7, EVOCATION, FALSE);
  spello(SPELL_HORIZIKAULS_BOOM, "horizikauls boom", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_AFFECTS,
         NULL, 1, 7, EVOCATION, FALSE);
  spello(SPELL_BURNING_HANDS, "burning hands", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 1, 7, EVOCATION, FALSE);
  spello(SPELL_FAERIE_FIRE, "faerie fire", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         NULL, 1, 7, EVOCATION, FALSE);
  spello(SPELL_PRODUCE_FLAME, "produce flame", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 3, 7, EVOCATION, FALSE);
  /* conjuration */
  spello(SPELL_ICE_DAGGER, "ice dagger", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 1, 7, CONJURATION, FALSE);
  spello(SPELL_MAGE_ARMOR, "mage armor", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You feel less protected.", 4, 7,
         CONJURATION, FALSE);
  spello(SPELL_OBSCURING_MIST, "obscuring mist", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_ROOM,
         "The obscuring mist begins to dissipate.", 3, 7, CONJURATION, FALSE);
  spello(SPELL_SUMMON_NATURES_ALLY_1, "natures ally i", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS, NULL, 4, 7, CONJURATION, FALSE);
  // summon creature 1 - shared
  /* necromancy */
  spello(SPELL_CHILL_TOUCH, "chill touch", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_AFFECTS,
         "You feel your strength return.", 1, 7, NECROMANCY, FALSE);
  spello(SPELL_RAY_OF_ENFEEBLEMENT, "ray of enfeeblement", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You feel your strength return.", 1, 7, NECROMANCY, FALSE);
  // negative energy ray - shared
  /* enchantment */
  spello(SPELL_CHARM_ANIMAL, "charm animal", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_MANUAL,
         "You feel more self-confident.", 4, 7, ENCHANTMENT, FALSE);
  spello(SPELL_CHARM, "charm person", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_MANUAL,
         "You feel more self-confident.", 4, 7, ENCHANTMENT, FALSE);
  spello(SPELL_ENCHANT_ITEM, "enchant item", 0, 0, 0, POS_FIGHTING,
         TAR_OBJ_INV, FALSE, MAG_MANUAL,
         NULL, 5, 7, ENCHANTMENT, FALSE);
  spello(SPELL_SLEEP, "sleep", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You feel less tired.", 4, 7, ENCHANTMENT, FALSE);
  /* illusion */
  spello(SPELL_COLOR_SPRAY, "color spray", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_AFFECTS,
         NULL, 1, 7, ILLUSION, FALSE);
  // scare - shared
  spello(SPELL_TRUE_STRIKE, "true strike", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel you are no longer able to strike true!", 0, 7, ILLUSION, FALSE);
  /* divination */
  spello(SPELL_IDENTIFY, "identify", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL,
         NULL, 5, 7, DIVINATION, FALSE);
  spello(SPELL_SHELGARNS_BLADE, "shelgarns blade", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS,
         NULL, 6, 7, DIVINATION, FALSE);
  spello(SPELL_GREASE, "grease", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You feel the grease spell wear off.", 1, 7, DIVINATION, FALSE);
  /* abjuration */
  // endure elements - shared
  // protect from evil - shared
  // protect from good - shared
  /* transmutation */
  spello(SPELL_EXPEDITIOUS_RETREAT, "expeditious retreat", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel less expeditious.", 0, 7,
         TRANSMUTATION, FALSE);
  spello(SPELL_GOODBERRY, "goodberry", 0, 0, 0, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_CREATIONS,
         NULL, 3, 7, TRANSMUTATION, FALSE);
  spello(SPELL_IRON_GUTS, "iron guts", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your guts feel less resilient.", 3, 7, TRANSMUTATION, FALSE);
  spello(SPELL_JUMP, "jump", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel your jumping ability return to normal.", 3, 7, TRANSMUTATION, FALSE);
  spello(SPELL_MAGIC_FANG, "magic fang", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your magic fang wears off.", 3, 7, TRANSMUTATION, FALSE);
  spello(SPELL_ENTANGLE, "entangle", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "The vines around your feet turn to dust.", 3, 7, TRANSMUTATION, FALSE);
  spello(SPELL_MAGIC_STONE, "magic stone", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_CREATIONS,
         NULL, 3, 7, TRANSMUTATION, FALSE);
  spello(SPELL_SHIELD, "shield", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your magical shield fades away.", 3, 7, TRANSMUTATION, FALSE);

  /* = =  2nd circle  = = */
  /* evocation */
  spello(SPELL_ACID_ARROW, "acid arrow", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_MANUAL,
         NULL, 2, 9, EVOCATION, FALSE);
  spello(SPELL_SHOCKING_GRASP, "shocking grasp", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 2, 9, EVOCATION, FALSE);
  spello(SPELL_SCORCHING_RAY, "scorching ray", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 2, 9, EVOCATION, FALSE);
  spello(SPELL_CONTINUAL_FLAME, "continual flame", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_CREATIONS,
         NULL, 5, 9, EVOCATION, FALSE);
  spello(SPELL_FLAME_BLADE, "flame blade", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 2, 9, EVOCATION, FALSE);
  spello(SPELL_FLAMING_SPHERE, "flaming sphere", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 5, 9, EVOCATION, FALSE);
  /* conjuration */
  // summon creature 2 - shared
  spello(SPELL_SUMMON_NATURES_ALLY_2, "natures ally ii", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS,
         NULL, 4, 9, CONJURATION, FALSE);
  spello(SPELL_SUMMON_SWARM, "summon swarm", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 5, 9, CONJURATION, FALSE);
  spello(SPELL_WEB, "web", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_AFFECTS,
         "You feel the sticky strands of the magical web dissolve.", 2, 9,
         CONJURATION, FALSE);
  /* necromancy */
  // blindness - shared
  spello(SPELL_FALSE_LIFE, "false life", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel your necromantic-life drain away.", 4, 9, NECROMANCY, FALSE);

  spello(SPELL_MASS_FALSE_LIFE, "mass false life", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_GROUPS,
         "You feel your necromantic-life drain away.", 4, 9, NECROMANCY, FALSE);

  /* enchantment */
  spello(SPELL_DAZE_MONSTER, "daze monster", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You no longer feel dazed.", 2, 9,
         ENCHANTMENT, FALSE);
  spello(SPELL_HIDEOUS_LAUGHTER, "hideous laughter", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You feel able to control your laughter again.", 2, 9,
         ENCHANTMENT, FALSE);
  spello(SPELL_HOLD_ANIMAL, "hold animal", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You feel able to control yourself again.", 3, 9,
         ENCHANTMENT, FALSE);
  spello(SPELL_TOUCH_OF_IDIOCY, "touch of idiocy", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You begin to feel less incompetent.", 2, 9,
         ENCHANTMENT, FALSE);
  spello(SPELL_CONFUSION, "confusion", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "The confusion fogging your mind has passed.", 2, 9,
         ENCHANTMENT, FALSE);
  /* illusion */
  spello(SPELL_BLUR, "blur", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel your blur spell wear off.", 2, 9, ILLUSION, FALSE);
  spello(SPELL_INVISIBLE, "invisibility", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_AFFECTS | MAG_ALTER_OBJS,
         "You feel yourself exposed.", 4, 9, ILLUSION, FALSE);
  spello(SPELL_MIRROR_IMAGE, "mirror image", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You watch as your images vanish.", 3, 9, ILLUSION, FALSE);
  /* divination */
  spello(SPELL_DETECT_INVIS, "detect invisibility", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your eyes stop tingling.", 1, 9, DIVINATION, FALSE);
  // detect magic - shared
  // darkness - shared
  /* abjuration */
  // resist energy
  spello(SPELL_ENERGY_SPHERE, "energy sphere", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 2, 9, ABJURATION, FALSE);
  /* transmutation */
  spello(SPELL_BARKSKIN, "barkskin", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel your barkskin wear off.", 3, 9, TRANSMUTATION, FALSE);
  // endurance - shared
  // strengrth - shared
  // grace - shared

  // 3rd cricle
  /* evocation */
  spello(SPELL_LIGHTNING_BOLT, "lightning bolt", 44, 29, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 3, 11, EVOCATION, FALSE);
  spello(SPELL_FIREBALL, "fireball", 44, 29, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 3, 11, EVOCATION, FALSE);
  spello(SPELL_WATER_BREATHE, "water breathe", 79, 64, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your water breathe enchantment fades away.", 7, 11, EVOCATION, FALSE);
  /* conjuration */
  spello(SPELL_SUMMON_NATURES_ALLY_3, "natures ally iii", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS,
         NULL, 5, 11, CONJURATION, FALSE);
  // summon creature 3 - shared
  spello(SPELL_PHANTOM_STEED, "phantom steed", 95, 80, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS,
         NULL, 7, 11, CONJURATION, FALSE);
  spello(SPELL_STINKING_CLOUD, "stinking cloud", 65, 50, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_ROOM,
         "You watch as the noxious gasses fade away.", 4, 11,
         CONJURATION, FALSE);
  /* necromancy */
  spello(SPELL_BLIGHT, "blight", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 4, 11, NECROMANCY, FALSE);
  spello(SPELL_CONTAGION, "contagion", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You feel much better as your disease wears off.", 5, 11, NECROMANCY, FALSE);
  spello(SPELL_HALT_UNDEAD, "halt undead", 65, 50, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         "You feel the necromantic halt spell fade away.", 5, 11,
         NECROMANCY, FALSE);
  spello(SPELL_VAMPIRIC_TOUCH, "vampiric touch", 44, 29, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_POINTS,
         NULL, 3, 11, NECROMANCY, FALSE);
  spello(SPELL_HEROISM, "deathly heroism", 30, 15, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your deathly heroism fades away.", 4, 11,
         NECROMANCY, FALSE);
  /* enchantment */
  spello(SPELL_FLY, "fly", 37, 22, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You drift slowly to the ground.", 3, 11, ENCHANTMENT, FALSE);
  spello(SPELL_HOLD_PERSON, "hold person", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You feel able to move again.", 3, 7,
         ENCHANTMENT, FALSE);
  spello(SPELL_DEEP_SLUMBER, "deep slumber", 58, 43, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You feel less tired.", 4, 11, ENCHANTMENT, FALSE);
  /* illusion */
  spello(SPELL_WALL_OF_FOG, "wall of fog", 50, 25, 5, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_ROOM,
         "The wall of fog blows away.", 6, 11, ILLUSION, FALSE);
  spello(SPELL_INVISIBILITY_SPHERE, "invisibility sphere", 58, 43, 1,
         POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS,
         NULL, 7, 11, ILLUSION, FALSE);
  // daylight - shared
  /* divination */
  spello(SPELL_CLAIRVOYANCE, "clairvoyance", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL,
         NULL, 5, 11,
         DIVINATION, FALSE);
  spello(SPELL_NON_DETECTION, "nondetection", 37, 22, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your non-detection spell wore off.", 6, 11, DIVINATION, FALSE);
  // dispel magic - shared
  /* abjuration */
  spello(SPELL_HASTE, "haste", 37, 22, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel your haste spell wear off.", 4, 11, ABJURATION, FALSE);
  spello(SPELL_SLOW, "slow", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You feel the slow spell wear off.", 4, 11,
         ABJURATION, FALSE);
  // circle against evil - shared
  // circle against good - shared
  /* transmutation */
  spello(SPELL_GREATER_MAGIC_FANG, "greater magic fang", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your greater magic fang wears off.", 4, 11, TRANSMUTATION, FALSE);
  spello(SPELL_SPIKE_GROWTH, "spike growth", 0, 0, 0, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_ROOM,
         "The large spikes retract back into the earth.", 5, 11, TRANSMUTATION, FALSE);
  // cunning - shared
  // wisdom - shared
  // charisma - shared

  // 4th circle
  /* evocation */
  spello(SPELL_LESSER_MISSILE_STORM, "lesser missile storm", 72, 57, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 4, 13, EVOCATION, FALSE);
  spello(SPELL_ICE_STORM, "ice storm", 58, 43, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 5, 13, EVOCATION, FALSE);
  spello(SPELL_FIRE_SHIELD, "fire shield", 37, 22, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
         "You watch your fire shield fade away.", 5, 13, EVOCATION, FALSE);
  spello(SPELL_COLD_SHIELD, "cold shield", 37, 22, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
         "You watch your cold shield fade away.", 5, 13, EVOCATION, FALSE);
  /* conjuration */
  spello(SPELL_BILLOWING_CLOUD, "billowing cloud", 65, 50, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_ROOM,
         "You watch as the thick billowing cloud dissipates.", 7, 13,
         CONJURATION, FALSE);
  spello(SPELL_SUMMON_NATURES_ALLY_4, "natures ally iv", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS,
         NULL, 6, 13, CONJURATION, FALSE);
  // summon creature 4 - shared
  /* necromancy */
  // curse - shared
  /* enchantment */
  // infra - shared
  // poison - shared
  /* illusion */
  spello(SPELL_GREATER_INVIS, "greater invisibility", 58, 43, 1,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE,
         MAG_AFFECTS | MAG_ALTER_OBJS, "You feel yourself exposed.", 8, 13,
         ILLUSION, FALSE);
  spello(SPELL_RAINBOW_PATTERN, "rainbow pattern", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You no longer feel dazed.", 5, 13,
         ILLUSION, FALSE);
  /* divination */
  spello(SPELL_WIZARD_EYE, "wizard eye", 65, 50, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_MANUAL,
         NULL, 6, 13, DIVINATION, FALSE);
  spello(SPELL_LOCATE_CREATURE, "locate creature", 58, 43, 1, POS_FIGHTING,
         TAR_CHAR_WORLD, FALSE, MAG_MANUAL,
         NULL, 12, 13, DIVINATION, FALSE);
  /* abjuration */
  spello(SPELL_FREE_MOVEMENT, "freedom of movement(inc)", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You are no longer able to move freely.", 3, 13, ABJURATION, FALSE);
  spello(SPELL_STONESKIN, "stone skin", 51, 36, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your skin returns to its normal texture.", 3, 13, ABJURATION, FALSE);
  spello(SPELL_MINOR_GLOBE, "minor globe", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "Your minor globe has faded away.", 8,
         13, ABJURATION, FALSE);
  // remove curse
  /* transmutation */
  spello(SPELL_ENLARGE_PERSON, "enlarge person", 37, 22, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel your enlargement spell wear off.", 8, 13, TRANSMUTATION, FALSE);
  spello(SPELL_SHRINK_PERSON, "shrink person", 37, 22, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel your shrink spell wear off.", 8, 13, TRANSMUTATION, FALSE);
  spello(SPELL_SPIKE_STONES, "spike stone", 0, 0, 0, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_ROOM,
         "The large spike stones morph back into their natural form.", 8, 13, TRANSMUTATION, FALSE);

  // 5th circle
  /* evocation */
  spello(SPELL_BALL_OF_LIGHTNING, "ball of lightning", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL, 5, 15, EVOCATION, FALSE);
  spello(SPELL_CALL_LIGHTNING_STORM, "call lightning storm", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS, NULL, 8, 15, EVOCATION, FALSE);
  spello(SPELL_HALLOW, "hallow", 0, 0, 0, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_ROOM, NULL, 8, 15, EVOCATION, FALSE);
  spello(SPELL_INTERPOSING_HAND, "interposing hand", 80, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You feel more optimistic.", 7, 15, EVOCATION, FALSE);
  spello(SPELL_UNHALLOW, "unhallow", 0, 0, 0, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_ROOM, NULL, 8, 15, EVOCATION, FALSE);
  spello(SPELL_WALL_OF_FIRE, "wall of fire", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_MANUAL, NULL, 7, 15, EVOCATION, FALSE);
  spello(SPELL_WALL_OF_FORCE, "wall of force", 65, 50, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_MANUAL, NULL, 6, 15, EVOCATION, FALSE);
  /* conjuration */
  spello(SPELL_SUMMON_NATURES_ALLY_5, "natures ally v", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS,
         NULL, 7, 15, CONJURATION, FALSE);
  spello(SPELL_CLOUDKILL, "cloudkill", 65, 50, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_MANUAL, NULL, 8, 15, CONJURATION, FALSE);
  spello(SPELL_INSECT_PLAGUE, "insect plague", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 9, 15, CONJURATION, FALSE);
  spello(SPELL_SUMMON_CREATURE_5, "summon creature v", 95, 80, 1,
         POS_FIGHTING, TAR_IGNORE, FALSE, MAG_SUMMONS, NULL, 9, 15, CONJURATION, FALSE);
  spello(SPELL_WALL_OF_THORNS, "wall of thorns", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_MANUAL, NULL, 7, 15, CONJURATION, FALSE);
  /* necromancy */
  spello(SPELL_DEATH_WARD, "death ward", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
         "You are no longer warded against the effects of death magic.", 7, 15, NECROMANCY, FALSE);
  spello(SPELL_SYMBOL_OF_PAIN, "symbol of pain", 58, 43, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS, NULL, 8, 15, NECROMANCY, FALSE);
  spello(SPELL_WAVES_OF_FATIGUE, "waves of fatigue", 65, 50, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS, "You feel the magical fatigue fade away.", 7,
         9, NECROMANCY, FALSE);
  /* enchantment */
  spello(SPELL_DOMINATE_PERSON, "dominate person", 51, 36, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_MANUAL,
         "You feel the domination effects wear off.", 10, 15, ENCHANTMENT, FALSE);
  spello(SPELL_FEEBLEMIND, "feeblemind", 80, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "Your enfeebled mind returns to normal.", 7, 15, ENCHANTMENT, FALSE);
  /* illusion */
  spello(SPELL_NIGHTMARE, "nightmare", 30, 15, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_AFFECTS,
         "You are able to shake the horrid nightmare.", 7, 15, ILLUSION, FALSE);
  spello(SPELL_MIND_FOG, "mind fog", 80, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "Your fogged mind returns to normal.", 7, 15, ILLUSION, FALSE);
  /* divination */
  spello(SPELL_FAITHFUL_HOUND, "faithful hound", 95, 80, 1,
         POS_FIGHTING, TAR_IGNORE, FALSE, MAG_SUMMONS, NULL, 9, 15, DIVINATION, FALSE);
  spello(SPELL_ACID_SHEATH, "acid sheath", 37, 22, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
         "You watch your acid sheath fade away.", 6, 15, DIVINATION, FALSE);
  /* abjuration */
  spello(SPELL_DISMISSAL, "dismissal", 51, 36, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_MANUAL, NULL, 7, 15,
         ABJURATION, FALSE);
  spello(SPELL_CONE_OF_COLD, "cone of cold", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL, 9, 15, ABJURATION, FALSE);
  /* transmutation */
  spello(SPELL_TELEKINESIS, "telekinesis", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL, 9, 15,
         TRANSMUTATION, FALSE);
  spello(SPELL_FIREBRAND, "firebrand", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL, 9, 15,
         TRANSMUTATION, FALSE);

  // 6th circle
  /* evocation */
  spello(SPELL_FREEZING_SPHERE, "freezing sphere", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL, 5, 17, EVOCATION, FALSE);
  /* conjuration */
  spello(SPELL_SUMMON_NATURES_ALLY_6, "natures ally vi", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS,
         NULL, 8, 17, CONJURATION, FALSE);
  spello(SPELL_ACID_FOG, "acid fog", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_ROOM,
         "You watch as the acid fog dissipates.", 7, 17,
         CONJURATION, FALSE);
  spello(SPELL_FIRE_SEEDS, "fire seeds", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_CREATIONS, NULL, 7, 17, CONJURATION, FALSE);
  spello(SPELL_TRANSPORT_VIA_PLANTS, "transport via plants", 0, 0, 0, POS_STANDING,
         TAR_OBJ_ROOM, FALSE, MAG_MANUAL, NULL, 8, 17, CONJURATION, FALSE);
  // summon creature 6 - shared
  /* necromancy */
  spello(SPELL_TRANSFORMATION, "transformation", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
         "You feel your transformation fade.", 5, 17, NECROMANCY, FALSE);
  spello(SPELL_CIRCLE_OF_DEATH, "circle of death", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_AREAS, NULL, 5, 17, NECROMANCY, FALSE);
  spello(SPELL_UNDEATH_TO_DEATH, "undeath to death", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_AREAS, NULL, 5, 17, NECROMANCY, FALSE);
  // sorcerer bloodline undead abilities
  spello(SPELL_GRASP_OF_THE_DEAD, "grasp of the dead", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_AREAS, NULL, 5, 17, NECROMANCY, FALSE);
  spello(SPELL_GRAVE_TOUCH, "grave touch", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_AFFECTS, NULL, 5, 17, NECROMANCY, FALSE);
  spello(SPELL_INCORPOREAL_FORM, "incorporeal form (undead bloodline)", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AREAS, NULL, 5, 17, NECROMANCY, FALSE);
  // eyebite - shared
  /* enchantment */
  spello(SPELL_MASS_HASTE, "mass haste", 0, 0, 0, POS_FIGHTING, TAR_IGNORE,
         FALSE, MAG_GROUPS, "The haste spell fades away.", 8, 17, ENCHANTMENT, FALSE);
  spello(SPELL_GREATER_HEROISM, "greater heroism", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "Your greater heroism fades away.",
         6, 17, ENCHANTMENT, FALSE);
  /* illusion */
  spello(SPELL_ANTI_MAGIC_FIELD, "anti magic field", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_ROOM,
         "You watch as the shimmering anti-magic field dissipates.", 7, 17,
         ILLUSION, FALSE);
  spello(SPELL_GREATER_MIRROR_IMAGE, "greater mirror image", 0, 0, 0,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
         "You watch as your images vanish.", 5, 17, ILLUSION, FALSE);
  /* divination */
  spello(SPELL_LOCATE_OBJECT, "locate object", 0, 0, 0, POS_FIGHTING,
         TAR_OBJ_WORLD, FALSE, MAG_MANUAL,
         NULL, 10, 17, DIVINATION, FALSE);
  spello(SPELL_TRUE_SEEING, "true seeing", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "Your eyes stop seeing true.", 5, 17,
         DIVINATION, FALSE);
  /* abjuration */
  spello(SPELL_GLOBE_OF_INVULN, "globe of invuln", 0, 0, 0,
         POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your globe of invulnerability has faded away.", 6, 17, ABJURATION, FALSE);
  spello(SPELL_GREATER_DISPELLING, "greater dispelling", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_MANUAL, NULL, 4, 17, ABJURATION, FALSE);
  /* transmutation */
  spello(SPELL_CLONE, "clone", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS,
         NULL, 9, 17, TRANSMUTATION, FALSE);
  spello(SPELL_SPELLSTAFF, "spellstaff", 0, 0, 0, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_MANUAL, NULL, 9, 17, TRANSMUTATION, FALSE);
  spello(SPELL_WATERWALK, "waterwalk", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your feet seem less buoyant.", 7, 17, TRANSMUTATION, FALSE);
  spello(SPELL_LEVITATE, "levitate", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "As the levitation expires, you begin to float downward.", 7, 17, TRANSMUTATION, FALSE);

  // 7th circle
  /* evocation */
  spello(SPELL_FIRE_STORM, "fire storm", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS, NULL, 7, 19, EVOCATION, FALSE);
  spello(SPELL_GRASPING_HAND, "grasping hand", 72, 57, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_AFFECTS,
         NULL, 6, 19, EVOCATION, FALSE); // grapples opponent
  spello(SPELL_MISSILE_STORM, "missile storm", 72, 57, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 6, 19, EVOCATION, FALSE);
  spello(SPELL_SUNBEAM, "sunbeam", 0, 0, 0,
         POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS | MAG_ROOM,
         "You feel a cloak of blindness dissolve.", 6, 19, EVOCATION, FALSE);
  /* conjuration */
  spello(SPELL_SUMMON_NATURES_ALLY_7, "natures ally vii", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS,
         NULL, 9, 19, CONJURATION, FALSE);
  spello(SPELL_CREEPING_DOOM, "creeping doom", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_MANUAL, NULL, 10, 19, CONJURATION, FALSE);
  spello(SPELL_SUMMON_CREATURE_7, "summon creature vii", 0, 0, 0,
         POS_FIGHTING, TAR_IGNORE, FALSE, MAG_SUMMONS, NULL, 10, 19, CONJURATION, FALSE);
  // control weather, enhances some spells (shared)
  /* necromancy */
  spello(SPELL_POWER_WORD_BLIND, "power word blind", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You feel a cloak of blindness dissolve.", 0, 19,
         NECROMANCY, FALSE);
  spello(SPELL_WAVES_OF_EXHAUSTION, "waves of exhaustion", 65, 50, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS, "You feel the magical exhaustion fade away.", 8,
         19, NECROMANCY, FALSE); // like waves of fatigue, but no save?
  /* enchantment */
  spello(SPELL_MASS_HOLD_PERSON, "mass hold person", 65, 50, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS, "You feel the magical hold fade away.", 8,
         19, ENCHANTMENT, FALSE);
  spello(SPELL_MASS_FLY, "mass fly", 0, 0, 0, POS_FIGHTING, TAR_IGNORE,
         FALSE, MAG_GROUPS, "The fly spell fades away.", 7, 19, ENCHANTMENT, FALSE);
  /* illusion */
  spello(SPELL_DISPLACEMENT, "displacement", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
         "You feel your displacement spell wear off.", 6, 19, ILLUSION, FALSE);
  spello(SPELL_PRISMATIC_SPRAY, "prismatic spray", 79, 64, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 7, 19, ILLUSION, FALSE);
  /* divination */
  spello(SPELL_POWER_WORD_STUN, "power word stun", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You no longer feel stunned.", 0, 19,
         DIVINATION, FALSE);
  spello(SPELL_PROTECT_FROM_SPELLS, "protection from spells", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel your spell protection wear off.", 6, 19, DIVINATION, FALSE);
  // detect poison - shared
  /* abjuration */
  spello(SPELL_THUNDERCLAP, "thunderclap", 79, 64, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 7, 19, ABJURATION, FALSE); // aoe damage and affect
  spello(SPELL_SPELL_MANTLE, "spell mantle", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel your spell mantle wear off.", 6, 19, ABJURATION, FALSE);
  /* transmutation */
  spello(SPELL_TELEPORT, "teleport", 72, 57, 1, POS_FIGHTING,
         TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL,
         NULL, 2, 19, TRANSMUTATION, FALSE);
  spello(SPELL_SHADOW_JUMP, "shadow jump", 72, 57, 1, POS_FIGHTING,
         TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL,
         NULL, 2, 19, TRANSMUTATION, FALSE);
  // mass wisdom - shared
  // mass charisma - shared
  // mass cunning - shared

  // 8th circle
  /* evocation */
  spello(SPELL_CLENCHED_FIST, "clenched fist", 72, 57, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 7, 21, EVOCATION, FALSE);
  spello(SPELL_CHAIN_LIGHTNING, "chain lightning", 79, 64, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 8, 21, EVOCATION, FALSE);
  spello(SPELL_WHIRLWIND, "whirlwind", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS, NULL, 8, 21, EVOCATION, FALSE);
  /* conjuration */
  spello(SPELL_SUMMON_NATURES_ALLY_8, "natures ally viii", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS,
         NULL, 10, 21, CONJURATION, FALSE);
  spello(SPELL_INCENDIARY_CLOUD, "incendiary cloud", 65, 50, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_MANUAL, NULL, 9, 21, CONJURATION, FALSE);
  spello(SPELL_SUMMON_CREATURE_8, "summon creature viii", 0, 0, 0,
         POS_FIGHTING, TAR_IGNORE, FALSE, MAG_SUMMONS, NULL, 11, 21, CONJURATION, FALSE);
  /* necromancy */
  spello(SPELL_FINGER_OF_DEATH, "finger of death", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 8, 21, NECROMANCY, FALSE);
  spello(SPELL_GREATER_ANIMATION, "greater animation", 72, 57, 1, POS_FIGHTING,
         TAR_OBJ_ROOM, FALSE, MAG_SUMMONS,
         NULL, 11, 21, NECROMANCY, FALSE);
  spello(SPELL_HORRID_WILTING, "horrid wilting", 79, 64, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 9, 21, NECROMANCY, FALSE);
  /* enchantment */
  spello(SPELL_IRRESISTIBLE_DANCE, "irresistible dance", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You no longer feel the urge to moonwalk.", 5, 21,
         ENCHANTMENT, FALSE);
  spello(SPELL_MASS_DOMINATION, "mass domination", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_MANUAL, "You no longer feel dominated.", 6, 21, ENCHANTMENT, FALSE);
  /* illusion */
  spello(SPELL_SCINT_PATTERN, "scint pattern", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "The pattern no longer traps you.", 5, 21,
         ILLUSION, FALSE);
  spello(SPELL_REFUGE, "refuge", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_MANUAL, NULL, 6, 21, ILLUSION, FALSE);
  /* divination */
  spello(SPELL_BANISH, "banish", 51, 36, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_MANUAL, NULL, 8, 21,
         DIVINATION, FALSE);
  spello(SPELL_SUNBURST, "sunburst", 72, 57, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS | MAG_ROOM,
         NULL, 7, 21, DIVINATION, FALSE);
  /* abjuration */
  spello(SPELL_SPELL_TURNING, "spell turning", 79, 64, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your spell-turning field fades away.", 8, 21, ABJURATION, FALSE);
  spello(SPELL_MIND_BLANK, "mind blank", 79, 64, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your mind-blank fades.", 8, 21, ABJURATION, FALSE);
  /* transmutation */
  spello(SPELL_CONTROL_PLANTS, "control plants", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_MANUAL,
         "You are able to control yourself again.", 7, 21, TRANSMUTATION, FALSE);
  spello(SPELL_IRONSKIN, "iron skin", 51, 36, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your skin loses its iron-like texture.", 4, 21, TRANSMUTATION, FALSE);
  spello(SPELL_PORTAL, "portal", 37, 22, 1, POS_FIGHTING, TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_CREATIONS, NULL, 12, 21, TRANSMUTATION, FALSE);

  // 9th circle
  /* evocation */
  spello(SPELL_METEOR_SWARM, "meteor swarm", 85, 70, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS, NULL, 9, 23, EVOCATION, FALSE);
  spello(SPELL_BLADE_OF_DISASTER, "blade of disaster", 0, 0, 0,
         POS_FIGHTING, TAR_IGNORE, FALSE, MAG_SUMMONS, NULL, 14, 23, EVOCATION, FALSE);
  /* conjuration */
  spello(SPELL_SUMMON_NATURES_ALLY_9, "natures ally ix", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_SUMMONS,
         NULL, 11, 23, CONJURATION, FALSE);
  spello(SPELL_ELEMENTAL_SWARM, "elemental swarm", 0, 0, 0, POS_FIGHTING, TAR_IGNORE,
         FALSE, MAG_SUMMONS, NULL, 12, 23, CONJURATION, FALSE);
  spello(SPELL_GATE, "gate", 51, 36, 1, POS_FIGHTING, TAR_IGNORE, FALSE, MAG_CREATIONS, NULL, 9, 23, CONJURATION, FALSE);
  spello(SPELL_SHAMBLER, "shambler", 0, 0, 0, POS_FIGHTING, TAR_IGNORE, FALSE,
         MAG_SUMMONS, NULL, 9, 23, CONJURATION, FALSE);
  spello(SPELL_SUMMON_CREATURE_9, "summon creature ix", 0, 0, 0,
         POS_FIGHTING, TAR_IGNORE, FALSE, MAG_SUMMONS, NULL, 12, 23, CONJURATION, FALSE);
  /* necromancy */
  //*energy drain, shared
  spello(SPELL_WAIL_OF_THE_BANSHEE, "wail of the banshee", 85, 70, 1,
         POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS, NULL, 10, 23, NECROMANCY, FALSE);
  /* enchantment */
  spello(SPELL_POWER_WORD_KILL, "power word kill", 72, 57, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 0, 23, EVOCATION, FALSE);
  spello(SPELL_ENFEEBLEMENT, "enfeeblement", 65, 50, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS, "You no longer feel enfeebled.", 4, 23,
         ENCHANTMENT, FALSE);
  /* illusion */
  spello(SPELL_WEIRD, "weird", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_AFFECTS,
         "The phantasmal killer stop chasing you.", 4, 23,
         ENCHANTMENT, FALSE);
  spello(SPELL_SHADOW_SHIELD, "shadow shield", 95, 80, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel the shadow shield dissipate.", 5, 23, ILLUSION, FALSE);

  spello(SPELL_SHADOW_WALK, "shadow walk", 95, 80, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_GROUPS,
         "You feel your synergy with the shadow realm fade.", 5, 23, ILLUSION, FALSE);

  /* divination */
  spello(SPELL_PRISMATIC_SPHERE, "prismatic sphere", 65, 50, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_MANUAL, NULL, 8, 23, DIVINATION, FALSE);
  spello(SPELL_IMPLODE, "implode", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_MANUAL,
         NULL, 3, 23, DIVINATION, FALSE);
  /* abjuration */
  spello(SPELL_TIMESTOP, "timestop", 95, 80, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
         "Time begins to move again.", 0, 23, ABJURATION, FALSE);
  spello(SPELL_GREATER_SPELL_MANTLE, "greater spell mantle", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
         "You feel your greater spell mantle wear off.", 8, 23, ABJURATION, FALSE);
  /* transmutation */
  spello(SPELL_POLYMORPH, "polymorph self", 58, 43, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_MANUAL, NULL, 9, 23, TRANSMUTATION, FALSE);
  spello(SPELL_MASS_ENHANCE, "mass enhance", 0, 0, 0, POS_FIGHTING, TAR_IGNORE,
         FALSE, MAG_GROUPS, "The physical enhancement spell wears off.", 2, 23,
         TRANSMUTATION, FALSE);

  // epic magical
  spello(SPELL_EPIC_MAGE_ARMOR, "epic mage armor", 95, 80, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
         "You feel less protected.", 4, 1, ABJURATION, FALSE);
  spello(SPELL_EPIC_WARDING, "epic warding", 95, 80, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your massive magical ward dissipates.", 4, 1, ABJURATION, FALSE);
  // end magical

  // divine spells
  // 1st circle
  spello(SPELL_CURE_LIGHT, "cure light", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS,
         NULL, 1, 8, CONJURATION, FALSE);
  spello(SPELL_CAUSE_LIGHT_WOUNDS, "cause light wound", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 2, 8, NOSCHOOL, FALSE);
  spello(SPELL_RIGHTEOUS_VIGOR, "righteous vigor", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel less accurate.", 4, 8, ENCHANTMENT, FALSE);
  spello(SPELL_LIFE_SHIELD, "life shield", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "The dome of positive energy flashes and disappears.", 4, 8, CONJURATION, FALSE);
  spello(SPELL_EFFORTLESS_ARMOR, "effortless armor", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
         "You feel less agile in your armor.", 4, 8, TRANSMUTATION, FALSE);
  spello(SPELL_FIRE_OF_ENTANGLEMENT, "fire of entanglement", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
         "The entangling flame around your weapons disappears.", 4, 8, EVOCATION, FALSE);
  spello(SPELL_ARMOR, "armor", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel less protected.", 4, 8, CONJURATION, FALSE);
  spello(SPELL_BESTOW_WEAPON_PROFICIENCY, "bestow weapon proficiency", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel less proficient.", 4, 8, ENCHANTMENT, FALSE);
  spello(SPELL_TACTICAL_ACUMEN, "tactical acumen", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_GROUPS,
         "You feel your enhances tactical abilities fade.", 4, 8, ENCHANTMENT, FALSE);
  spello(SPELL_VEIL_OF_POSITIVE_ENERGY, "veil of positive energy", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
         "You feel the veil of positive energy fade.", 4, 8, ABJURATION, FALSE);
  spello(SPELL_SUN_METAL, "sun metal", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "The flames around your weapon dissipate.", 4, 8, CONJURATION, FALSE);
  spello(SPELL_STUNNING_BARRIER, "stunning barrier", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
         "You feel the stunning barrier dissipate.", 4, 8, CONJURATION, FALSE);
  spello(SPELL_SHIELD_OF_FORTIFICATION, "shield of fortification", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel less fortified.", 4, 8, CONJURATION, FALSE);
  spello(SPELL_RESISTANCE, "resistance", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You feel less resistant.", 4, 8, ABJURATION, FALSE);
  spello(SPELL_HONEYED_TONGUE, "honeyed tongue", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "You feel your silver tongue become less influential.", 4, 8, TRANSMUTATION, FALSE);
  spello(SPELL_HEDGING_WEAPONS, "hedging weapons", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS, "The last of your hedging weapons dissipates.", 4, 8, ABJURATION, FALSE);
  spello(SPELL_REMOVE_FEAR, "remove fear", 44, 29, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS, NULL, 3, 8, CONJURATION, FALSE);
  spello(SPELL_LESSER_RESTORATION, "lesser restoration", 44, 29, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS, NULL, 3, 8, CONJURATION, FALSE);
  spello(SPELL_RESTORATION, "restoration", 44, 29, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS, NULL, 5, 15, CONJURATION, FALSE);
  spello(SPELL_DOOM, "doom", 0, 0, 0, POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_AFFECTS, "You are no longer filled with feelings of doom.", 2, 8, NECROMANCY, FALSE);
  spello(SPELL_DIVINE_FAVOR, "divine favor", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, "You feel the divine favor subside.", 4, 8, EVOCATION, FALSE);
  spello(SPELL_SILENCE, "silence", 0, 0, 0, POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You feel the power that is muting you fade.", 3, 11, ILLUSION, FALSE); // wiz2, cle3
  spello(SPELL_CAUSE_LIGHT_WOUNDS, "cause light wound", 30, 15, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE, NULL, 2, 8, NOSCHOOL, FALSE);
  spello(SPELL_ARMOR, "armor", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE,
         MAG_AFFECTS, "You feel less protected.", 4, 8, CONJURATION, FALSE);
  spello(SPELL_REMOVE_FEAR, "remove fear", 44, 29, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS,
         NULL, 3, 8, NOSCHOOL, FALSE);
  spello(SPELL_DOOM, "doom", 0, 0, 0, POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_AFFECTS,
         "You are no longer filled with feelings of doom.", 2, 8, NECROMANCY, FALSE);
  // endurance - shared
  // negative energy ray - shared
  // endure elements - shared
  // protect from evil - shared
  // protect from good - shared
  // summon creature i - shared
  // strength - shared
  // grace - shared

  // 2nd circle
  spello(SPELL_CREATE_FOOD, "create food", 37, 22, 1, POS_FIGHTING, TAR_IGNORE,
         FALSE, MAG_CREATIONS, NULL, 2, 10, NOSCHOOL, FALSE);
  spello(SPELL_CREATE_WATER, "create water", 37, 22, 1, POS_FIGHTING,
         TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL, NULL, 2, 10, NOSCHOOL, FALSE);
  spello(SPELL_CURE_MODERATE, "cure moderate", 30, 15, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_POINTS, NULL, 2, 10, NOSCHOOL, FALSE);
  spello(SPELL_CAUSE_MODERATE_WOUNDS, "cause moderate wound", 37, 22, 1,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 3, 10, NOSCHOOL, FALSE);
  spello(SPELL_AUGURY, "augury", 30, 15, 1, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_MANUAL, NULL, 2, 10, NOSCHOOL, FALSE);
  // detect poison - shared
  // scare - shared
  // summon creature ii - shared
  // detect magic - shared
  // darkness - shared
  // resist energy - shared
  // wisdom - shared
  // charisma - shared

  // 3rd circle
  spello(SPELL_DETECT_ALIGN, "detect alignment", 44, 29, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel less aware.", 3, 12, NOSCHOOL, FALSE);
  spello(SPELL_CURE_BLIND, "cure blind", 44, 29, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS,
         NULL, 3, 12, NOSCHOOL, FALSE);
  spello(SPELL_BLESS, "bless", 44, 29, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE, MAG_AFFECTS | MAG_ALTER_OBJS,
         "You feel less righteous.", 3, 12, NOSCHOOL, FALSE);
  spello(SPELL_CURE_SERIOUS, "cure serious", 30, 15, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_POINTS, NULL, 3, 12, NOSCHOOL, FALSE);
  spello(SPELL_CAUSE_SERIOUS_WOUNDS, "cause serious wound", 44, 29, 1,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 4, 12, NOSCHOOL, FALSE);
  spello(SPELL_CURE_DEAFNESS, "cure deafness", 44, 29, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS,
         NULL, 5, 12, NOSCHOOL, FALSE);
  spello(SPELL_FAERIE_FOG, "faerie fog", 65, 50, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS, NULL, 4, 12, NOSCHOOL, FALSE);
  // summon creature 3 - shared
  // deafness - shared
  // cunning - shared
  // dispel magic - shared
  // animate dead - shared

  // 4th circle
  spello(SPELL_CURE_CRITIC, "cure critic", 51, 36, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_POINTS,
         NULL, 3, 14, NOSCHOOL, FALSE);
  spello(SPELL_CAUSE_CRITICAL_WOUNDS, "cause critical wound", 51, 36, 1,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 5, 14, NOSCHOOL, FALSE);
  spello(SPELL_MASS_CURE_LIGHT, "mass cure light", 85, 70, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_GROUPS,
         NULL, 5, 14, NOSCHOOL, FALSE);
  spello(SPELL_AID, "aid", 44, 29, 1, POS_FIGHTING, TAR_IGNORE, FALSE,
         MAG_GROUPS, "You feel the aid spell fade away.", 5, 10, NOSCHOOL, FALSE);
  spello(SPELL_BRAVERY, "bravery", 44, 29, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS | MAG_AFFECTS,
         "You feel your bravery spell wear off.", 8, 14, NOSCHOOL, FALSE);
  // summon creature iv - shared
  // remove curse - shared
  // infravision - shared
  // circle against evil - shared
  // circle against good - shared
  // curse - shared
  // daylight - shared

  // 5th circle
  spello(SPELL_REMOVE_POISON, "remove poison", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_UNAFFECTS | MAG_ALTER_OBJS,
         NULL, 7, 16, NOSCHOOL, FALSE);
  /* protection from evil (declared above) */
  spello(SPELL_GROUP_ARMOR, "group armor", 58, 43, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_GROUPS,
         NULL, 5, 16, NOSCHOOL, FALSE);
  spello(SPELL_FLAME_STRIKE, "flame strike", 58, 43, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 6, 16, NOSCHOOL, FALSE);
  /* protection from good (delcared above) */
  spello(SPELL_MASS_CURE_MODERATE, "mass cure moderate", 85, 70, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_GROUPS,
         NULL, 6, 16, NOSCHOOL, FALSE);
  spello(SPELL_REGENERATION, "regeneration", 58, 43, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | MAG_POINTS,
         "You feel the regeneration spell wear off.", 5, 16, NOSCHOOL, FALSE);
  spello(SPELL_FREE_MOVEMENT, "free movement", 58, 43, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | MAG_UNAFFECTS,
         "You feel the free movement spell wear off.", 5, 16, NOSCHOOL, FALSE);
  spello(SPELL_STRENGTHEN_BONE, "strengthen bones", 58, 43, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel your undead bones weaken again.", 5, 16, NOSCHOOL, FALSE);
  // poison - shared
  // summon creature 5 - shared
  // waterbreath - shared
  // waterwalk - shared
  // levitate - shared

  // 6th circle
  spello(SPELL_DISPEL_EVIL, "dispel evil", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 5, 18, NOSCHOOL, FALSE);
  spello(SPELL_HARM, "harm", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 7, 18, NOSCHOOL, FALSE);
  spello(SPELL_HEAL, "heal", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_POINTS | MAG_UNAFFECTS,
         NULL, 5, 18, NOSCHOOL, FALSE);
  spello(SPELL_HEAL_MOUNT, "heal mount", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_POINTS | MAG_UNAFFECTS,
         NULL, 5, 18, NOSCHOOL, FALSE);
  spello(SPELL_DISPEL_GOOD, "dispel good", 65, 50, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 5, 18, NOSCHOOL, FALSE);
  spello(SPELL_MASS_CURE_SERIOUS, "mass cure serious", 85, 70, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_GROUPS,
         NULL, 6, 18, NOSCHOOL, FALSE);
  spello(SPELL_PRAYER, "prayer", 44, 29, 1, POS_FIGHTING, TAR_IGNORE, FALSE,
         MAG_GROUPS, "You feel the aid spell fade away.", 8, 18, NOSCHOOL, FALSE);
  spello(SPELL_REMOVE_DISEASE, "remove disease", 44, 29, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS,
         NULL, 7, 18, NOSCHOOL, FALSE);
  // summon creature 6 - shared
  // eyebite - shared
  // mass wisdom - shared
  // mass charisma - shared
  // mass cunning - shared

  // 7th circle
  spello(SPELL_CALL_LIGHTNING, "call lightning", 72, 57, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 8, 20, NOSCHOOL, FALSE);
  spello(SPELL_SUMMON, "summon", 72, 57, 1, POS_FIGHTING,
         TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL,
         NULL, 10, 20, NOSCHOOL, FALSE);
  spello(SPELL_WORD_OF_RECALL, "word of recall", 72, 57, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_MANUAL,
         NULL, 0, 20, NOSCHOOL, FALSE);
  spello(SPELL_MASS_CURE_CRIT, "mass cure critic", 85, 70, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_GROUPS,
         NULL, 7, 20, NOSCHOOL, FALSE);
  spello(SPELL_SENSE_LIFE, "sense life", 79, 64, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel less aware of your surroundings.", 8, 20, NOSCHOOL, FALSE);
  spello(SPELL_BLADE_BARRIER, "blade barrier", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_ROOM,
         "You watch as the barrier of blades dissipates.", 7, 20,
         CONJURATION, FALSE);
  spello(SPELL_BATTLETIDE, "battletide", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
         "You feel the battletide fade.", 10, 20, NOSCHOOL, FALSE);
  spello(SPELL_SPELL_RESISTANCE, "magic resistance", 79, 64, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You feel your spell resistance fade.", 8, 20, NOSCHOOL, FALSE);
  // control weather - shared
  // summon creature 7 - shared
  // greater dispelling - shared
  // mass enhance - shared

  // 8th circle
  spello(SPELL_SANCTUARY, "sanctuary", 79, 64, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "The white aura around your body fades.", 8, 22, NOSCHOOL, FALSE);
  spello(SPELL_DESTRUCTION, "destruction", 79, 64, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 9, 22, NOSCHOOL, FALSE);
  spello(SPELL_WORD_OF_FAITH, "word of faith", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You no longer feel divinely inflicted.", 0, 22,
         NOSCHOOL, FALSE);
  spello(SPELL_DIMENSIONAL_LOCK, "dimensional lock", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
         "You feel locked to this dimension.", 0, 22,
         NOSCHOOL, FALSE);
  spello(SPELL_SALVATION, "salvation", 79, 64, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_MANUAL,
         NULL, 8, 22, NOSCHOOL, FALSE);
  spello(SPELL_SPRING_OF_LIFE, "spring of life", 37, 22, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_CREATIONS, NULL, 14, 22, NOSCHOOL, FALSE);
  // druid spell
  spello(SPELL_ANIMAL_SHAPES, "animal shapes", 85, 70, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_GROUPS,
         "The primal spell wears off!", 5, 22, NOSCHOOL, FALSE);

  // 9th circle
  spello(SPELL_EARTHQUAKE, "earthquake", 85, 70, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 10, 25, NOSCHOOL, FALSE);
  spello(SPELL_PLANE_SHIFT, "plane shift", 85, 70, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_MANUAL, NULL, 2, 25, NOSCHOOL, FALSE);
  spello(SPELL_GROUP_HEAL, "group heal", 85, 70, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_GROUPS,
         NULL, 5, 25, NOSCHOOL, FALSE);
  spello(SPELL_GROUP_SUMMON, "group summon", 85, 70, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_MANUAL,
         NULL, 5, 25, NOSCHOOL, FALSE);
  spello(SPELL_STORM_OF_VENGEANCE, "storm of vengeance", 85, 70, 1, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_MANUAL,
         NULL, 12, 25, NOSCHOOL, FALSE);
  // energy drain - shared

  // epic divine
  // end divine

  /* NON-castable spells should appear below here. */
  spello(SPELL_ACID, "acid", 79, 64, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_MASSES,
         NULL, 8, 12, EVOCATION, FALSE);
  spello(SPELL_ASHIELD_DAM, "acidsheath damage", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AFFECTS,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(SPELL_BLADES, "blades", 79, 64, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_MASSES,
         NULL, 8, 12, NOSCHOOL, FALSE);
  spello(SPELL_CSHIELD_DAM, "coldshield damage", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AFFECTS,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(SPELL_DEATHCLOUD, "deathcloud", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(SPELL_GENERIC_AOE, "aoe attack", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(SPELL_FIRE_BREATHE, "fire breathe", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(SPELL_FROST_BREATHE, "frost breathe", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(SPELL_LIGHTNING_BREATHE, "lightning breathe", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(SPELL_ACID_BREATHE, "acid breathe", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(SPELL_POISON_BREATHE, "poison breathe", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(SPELL_DRAGONFEAR, "dragon fear", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(SPELL_DRACONIC_BLOODLINE_BREATHWEAPON, "draconic bloodline breath weapon", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 0, 0, NOSCHOOL, FALSE);

  spello(SPELL_PROTECTION_FROM_ENERGY, "protection from energy", 79, 64, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your protection from energy expires.", 7, 11, ABJURATION, FALSE);
  spello(SPELL_COMMUNAL_PROTECTION_FROM_ENERGY, "communal protection from energy", 79, 64, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_GROUPS,
         NULL, 9, 13, ABJURATION, FALSE);

  spello(SPELL_SEARING_LIGHT, "searing light", 44, 29, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         NULL, 3, 11, EVOCATION, FALSE);

  spello(SPELL_DIVINE_POWER, "divine power", 37, 22, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
         "You feel your divine power ebb away.", 5, 13, EVOCATION, FALSE);

  spello(SPELL_AIR_WALK, "air walk", 37, 22, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You drift slowly to the ground.", 3, 11, TRANSMUTATION, FALSE);

  spello(SPELL_GASEOUS_FORM, "gaseous form", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "You return to your normal solid form.", 4, 11, TRANSMUTATION, FALSE);

  spello(SPELL_WIND_WALL, "wind wall", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "The wall of swirling wind dissipates.", 3, 7, EVOCATION, FALSE);

  spello(SPELL_KEEN_EDGE, "keen edge", 79, 64, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your weapons lose their keen edge.", 7, 11, TRANSMUTATION, FALSE);
  spello(SPELL_WEAPON_OF_IMPACT, "weapon of impact", 79, 64, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your weapons stop glowing blue.", 7, 11, TRANSMUTATION, FALSE);

  spello(SPELL_SPIRITUAL_WEAPON, "spiritual weapon", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL,
         "Your spiritual weapon blinks out of existence.", 5, 9, EVOCATION, FALSE);
  spello(SPELL_DANCING_WEAPON, "dancing weapon", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL,
         "Your dancing weapon blinks out of existence.", 5, 9, EVOCATION, FALSE);

  spello(SPELL_HOLY_JAVELIN, "holy javelin", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF, TRUE, MAG_MANUAL,
         NULL, 3, 11, CONJURATION, FALSE);

  spello(SPELL_INVISIBILITY_PURGE, "invisibility purge", 79, 64, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AREAS, NULL, 7, 11, EVOCATION, FALSE);

  spello(SPELL_UNDETECTABLE_ALIGNMENT, "undetectable alignment", 0, 0, 0, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your alignment is no longer magically hidden.", 2, 11, ABJURATION, FALSE); // wizard 1, cleric 1

  spello(SPELL_WEAPON_OF_AWE, "weapon of awe", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your weapon stops glowing bright yellow.", 3, 9, TRANSMUTATION, FALSE);
  spello(SPELL_AFFECT_WEAPON_OF_AWE, "shaken by weapon of awe", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_MANUAL,
         "You no longer feel shaken in awe.", 3, 9, TRANSMUTATION, FALSE);

  spello(SPELL_BLINDING_RAY, "blinding ray", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_LOOPS,
         "You are no longer blinded.", 2, 9, EVOCATION, FALSE);

  spello(SPELL_GREATER_MAGIC_WEAPON, "greater magic weapon", 79, 64, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your weapons stop glowing with magical energy.", 7, 11, TRANSMUTATION, FALSE);
  spello(SPELL_MAGIC_VESTMENT, "magic vestment", 79, 64, 1, POS_FIGHTING,
         TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "Your armor stops glowing with magical energy.", 7, 11, TRANSMUTATION, FALSE);

  spello(SPELL_LITANY_OF_DEFENSE, "litany of defense", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "The litany of defense expires.", 4, 8, TRANSMUTATION, FALSE);
  spello(SPELL_LITANY_OF_RIGHTEOUSNESS, "litany of righteousness", 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
         "The litany of righteousness expires.", 4, 8, EVOCATION, FALSE);

  spello(ABILITY_AFFECT_BANE_WEAPON, "bane weapon", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_AFFECTS,
         "You weapons are no longer enchanted with the bane effect.", 1, 1, NOSCHOOL, FALSE);

  spello(ABILITY_AFFECT_TRUE_JUDGEMENT, "true judgement", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_AFFECTS,
         "You are no longer preparing a true judgement.", 1, 1, NOSCHOOL, FALSE);

  spello(SPELL_REMOVE_PARALYSIS, "remove paralysis", 44, 29, 1, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS, NULL, 3, 8, CONJURATION, FALSE);

  spello(RACIAL_LICH_FEAR, "lich fear", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         "A lich fear effect has expired.", 0, 0, NOSCHOOL, FALSE);
  spello(FEAT_LICH_TOUCH, "lich touch", 0, 0, 0, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
         "A lich touch effect has expired.", 0, 0, NOSCHOOL, FALSE);
  spello(RACIAL_LICH_REJUV, "lich rejuvenation", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AFFECTS,
         "A lich rejuvenation effect has expired.", 0, 0, NOSCHOOL, FALSE);

  spello(BLACKGUARD_CRUELTY_AFFECTS, "blackguard cruelty", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AFFECTS, "A blackguard cruelty effect has expired.", 0, 0, NOSCHOOL, FALSE);

  spello(PALADIN_MERCY_INJURED_FAST_HEALING, "paladin mercy fast healing", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_AREAS, NULL, 0, 0, NOSCHOOL, FALSE);

  spello(ABILITY_CHANNEL_POSITIVE_ENERGY, "channel positive energy", 85, 70, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS | MAG_GROUPS, NULL, 9, 23, NOSCHOOL, FALSE);
  spello(ABILITY_CHANNEL_NEGATIVE_ENERGY, "channel negative energy", 85, 70, 1, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS | MAG_GROUPS, NULL, 9, 23, NOSCHOOL, FALSE);

  spello(SPELL_FSHIELD_DAM, "fireshield damage", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AFFECTS,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(SPELL_ESHIELD_DAM, "EShield damage", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AFFECTS,
         NULL, 0, 0, NOSCHOOL, FALSE);
  /* innate darkness spell, room events testing spell as well */
  spello(SPELL_I_DARKNESS, "darkness", 0, 0, 0, POS_STANDING,
         TAR_IGNORE, FALSE, MAG_ROOM,
         "The cloak of darkness in the area dissolves.", 5, 6, NOSCHOOL, FALSE);

  spello(SPELL_AFFECT_MIND_TRAP_NAUSEA, "mind trap", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_AFFECTS,
         "The mind trap's nausea fades.", 1, 1, NOSCHOOL, FALSE);
  spello(SPELL_AFFECT_STUNNING_BARRIER, "stunning barrier effect", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_AFFECTS,
         "You are no longer stunned by a stunning barrier.", 1, 1, CONJURATION, FALSE);
  spello(AFFECT_ENTANGLING_FLAMES, "entangling flames effect", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_AFFECTS,
         "You are no longer entangled.", 1, 1, EVOCATION, FALSE);
  spello(SPELL_EFFECT_DAZZLED, "dazzled effect", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_AFFECTS,
         "You are no longer dazzled.", 1, 1, EVOCATION, FALSE);

  spello(PSIONIC_ABILITY_PSIONIC_FOCUS, "psionic focus", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_AFFECTS,
         "Your psionic focus expires.", 1, 1, NOSCHOOL, FALSE);
  spello(PSIONIC_ABILITY_DOUBLE_MANIFESTATION, "double manifestation", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_AFFECTS,
         "Your double manifest expires.", 1, 1, NOSCHOOL, FALSE);

  spello(SPELL_AFFECT_DEATH_ATTACK, "death attack paralyzation", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, FALSE, MAG_AFFECTS,
         "You are no longer paralyzed from a death attack.", 1, 1, EVOCATION, FALSE);

  /*
spello(SPELL_IDENTIFY, "identify", 0, 0, 0, 0,
    TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL,
    NULL, 0, 0, NOSCHOOL, FALSE);
*/

  spello(SKILL_SURPRISE_ACCURACY, "surprise accuracy", 0, 0, 0, POS_STANDING, // 529
         TAR_IGNORE, FALSE, 0,
         "The effects of your surprise accuracy have expired.", 0, 0, NOSCHOOL, FALSE);
  spello(SKILL_POWERFUL_BLOW, "powerful blow", 0, 0, 0, POS_STANDING, // 530
         TAR_IGNORE, FALSE, 0,
         "The effects of your powerful blow have expired.", 0, 0, NOSCHOOL, FALSE);
  spello(SPELL_INCENDIARY, "incendiary flames", 0, 0, 0, POS_FIGHTING,
         TAR_IGNORE, TRUE, MAG_AREAS,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(SPELL_STENCH, "stench", 65, 50, 1, POS_DEAD,
         TAR_IGNORE, FALSE, MAG_MASSES,
         "Your nausea from the noxious gas passes.", 4, 7,
         CONJURATION, FALSE);
  spello(BOMB_AFFECT_ACID, "acid bomb", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(BOMB_AFFECT_BLINDING, "blinding bomb", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(BOMB_AFFECT_BONESHARD, "boneshard bomb", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(BOMB_AFFECT_CONCUSSIVE, "concussive bomb", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(BOMB_AFFECT_CONFUSION, "confusion bomb", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(BOMB_AFFECT_FIRE_BRAND, "fire brand bomb", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(BOMB_AFFECT_FORCE, "force bomb", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(BOMB_AFFECT_FROST, "frost bomb", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(BOMB_AFFECT_HOLY, "holy bomb", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(BOMB_AFFECT_IMMOLATION, "immolation bomb", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(BOMB_AFFECT_PROFANE, "profane bomb", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(BOMB_AFFECT_SHOCK, "shock bomb", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(BOMB_AFFECT_STICKY, "sticky bomb", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(BOMB_AFFECT_SUNLIGHT, "sunlight bomb", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(BOMB_AFFECT_TANGLEFOOT, "tanglefoot bomb", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);
  spello(ALC_DISC_AFFECT_PSYCHOKINETIC, "psychokinetic tincture", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);

  spello(PSYCHOKINETIC_FEAR, "psychokinetic fear", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);

  spello(WEAPON_POISON_BLACK_ADDER_VENOM, "black adder venom", 85, 70, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_DAMAGE, "The black adder poison fully passes through your system.",
         9, 23, NOSCHOOL, FALSE);

  spello(SPELL_DG_AFFECT, "Afflicted", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, TRUE, 0,
         NULL, 0, 0, NOSCHOOL, FALSE);
}

void display_shadowcast_spells(struct char_data *ch)
{

  int i = 0, circle = 0, max_circle = 4;
  bool found = false;

  if (!ch)
    return;

  // shadowdancers can't cast anything until level 3
  if (CLASS_LEVEL(ch, CLASS_SHADOW_DANCER) < 3)
  {
    send_to_char(ch, "You can't shadowcast anything.\r\n");
    return;
  }

  if (CLASS_LEVEL(ch, CLASS_SHADOW_DANCER) >= 10)
  {
    max_circle = 7;
  }
  else if (HAS_REAL_FEAT(ch, FEAT_SHADOW_POWER))
  {
    max_circle = 4;
  }
  else if (HAS_REAL_FEAT(ch, FEAT_SHADOW_JUMP))
  {
    max_circle = 4;
  }
  else if (HAS_REAL_FEAT(ch, FEAT_SHADOW_CALL))
  {
    max_circle = 3;
  }
  else if (HAS_REAL_FEAT(ch, FEAT_SHADOW_ILLUSION))
  {
    max_circle = 2;
  }

  send_to_char(ch, "\tMShadowcast Spells Available\tn\r\n");

  for (circle = 1; circle <= max_circle; circle++)
  {
    found = false;
    for (i = 0; i < NUM_SPELLS; i++)
    {
      if (circle == compute_spells_circle(CLASS_WIZARD, i, 0, 0))
      {
        if (i == SPELL_MIRROR_IMAGE && HAS_REAL_FEAT(ch, FEAT_SHADOW_ILLUSION))
        {
          if (!found)
            send_to_char(ch, "\r\nSpell Circle %d\r\n", circle);
          found = true;
          send_to_char(ch, "%-22s (shadow illusion)\r\n", spell_info[i].name);
        }
        else if (i == SPELL_SHADOW_JUMP && HAS_REAL_FEAT(ch, FEAT_SHADOW_JUMP))
        {
          if (!found)
            send_to_char(ch, "\r\nSpell Circle %d\r\n", circle);
          found = true;
          send_to_char(ch, "%-22s (shadow jump)\r\n", spell_info[i].name);
        }
        else if (spell_info[i].schoolOfMagic == CONJURATION && HAS_REAL_FEAT(ch, FEAT_SHADOW_CALL))
        {
          if (CLASS_LEVEL(ch, CLASS_SHADOW_DANCER) < 10 && circle > 3)
            continue;
          if (circle > 6)
            continue;
          if (!found)
            send_to_char(ch, "\r\nSpell Circle %d\r\n", circle);
          found = true;
          send_to_char(ch, "%-22s (shadow call)\r\n", spell_info[i].name);
        }
        else if (spell_info[i].schoolOfMagic == EVOCATION && HAS_REAL_FEAT(ch, FEAT_SHADOW_POWER))
        {
          if (CLASS_LEVEL(ch, CLASS_SHADOW_DANCER) < 10 && circle > 4)
            continue;
          if (circle > 7)
            continue;
          if (!found)
            send_to_char(ch, "\r\nSpell Circle %d\r\n", circle);
          found = true;
          send_to_char(ch, "%-22s (shadow power)\r\n", spell_info[i].name);
        }
      }
    }
  }
}

/* must be at end of file */
#undef SINFO
/**************************/
