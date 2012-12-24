/**************************************************************************
*  File: spell_parser.c                                    Part of tbaMUD *
*  Usage: Top-level magic routines; outside points of entry to magic sys. *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
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
#include "fight.h"  /* for hit() */
#include "constants.h"
#include "mud_event.h"
#include "spec_procs.h"

#define SINFO spell_info[spellnum]

/* Global Variables definitions, used elsewhere */
struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];
char cast_arg2[MAX_INPUT_LENGTH];
const char *unused_spellname = "!UNUSED!"; /* So we can get &unused_spellname */
const char *unused_wearoff = "!UNUSED WEAROFF!"; /* So we can get &unused_wearoff */

/* Local (File Scope) Function Prototypes */
static void say_spell(struct char_data *ch, int spellnum, struct char_data *tch,
	struct obj_data *tobj, bool start);
static void spello(int spl, const char *name, int max_mana, int min_mana,
	int mana_change, int minpos, int targets, int violent, int routines,
	const char *wearoff, int time, int memtime, int school);
//static int mag_manacost(struct char_data *ch, int spellnum);

/* Local (File Scope) Variables */
struct syllable {
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
  {"a", "i"}, {"b", "v"}, {"c", "q"}, {"d", "m"}, {"e", "o"}, {"f", "y"}, {"g", "t"},
  {"h", "p"}, {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"},
  {"o", "a"}, {"p", "s"}, {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"}, {"u", "e"},
  {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"}, {"z", "k"}, {"", ""},
  {"1", "echad"},
  {"2", "shtayim"},
  {"3", "shelosh"},
  {"4", "arba"},
  {"5", "chamesh"},
  {"6", "sheish"},
  {"7", "shevah"},
  {"8", "shmoneh"},
  {"9", "teisha"},
  {"0", "efes"}
};


/* may use this for mobs to control their casting
static int mag_manacost(struct char_data *ch, int spellnum)
{

  return (MAX(SINFO.mana_max - (SINFO.mana_change *
		    (GET_LEVEL(ch) - SINFO.min_level[(int) GET_CLASS(ch)])),
	     SINFO.mana_min) / 2);
}
*/


static void say_spell(struct char_data *ch, int spellnum, struct char_data *tch,
	            struct obj_data *tobj, bool start)
{
  char lbuf[256], buf[256], buf1[256], buf2[256];	/* FIXME */
  const char *format;
  struct char_data *i;
  int j, ofs = 0, dc_of_id = 0, attempt = 0;

  dc_of_id = 20;  //DC of identifying the spell

  *buf = '\0';
  strlcpy(lbuf, skill_name(spellnum), sizeof(lbuf));

  while (lbuf[ofs]) {
    for (j = 0; *(syls[j].org); j++) {
      if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
	strcat(buf, syls[j].news);	/* strcat: BAD */
	ofs += strlen(syls[j].org);
        break;
      }
    }
    /* i.e., we didn't find a match in syls[] */
    if (!*syls[j].org) {
      log("No entry in syllable table for substring of '%s'", lbuf);
      ofs++;
    }
  }

  if (tch != NULL && IN_ROOM(tch) == IN_ROOM(ch)) {
    if (tch == ch) {
      if (!start)
        format = "\tn$n \tccloses $s eyes and utters the words, '\tC%s\tc'.\tn";
      else
        format =
"\tn$n \tcweaves $s hands in an \tmintricate\tc pattern and begins to chant the words, '\tC%s\tc'.\tn";
    } else {
      if (!start)
        format = "\tn$n \tcstares at \tn$N\tc and utters the words, '\tC%s\tc'.\tn";
      else
        format =
"\tn$n \tcweaves $s hands in an \tmintricate\tc pattern and begins to chant the words, '\tC%s\tc' at \tn$N\tc.\tn";
    } 
  } else if (tobj != NULL &&
	     ((IN_ROOM(tobj) == IN_ROOM(ch)) || (tobj->carried_by == ch))) {
    if (!start)
      format = "\tn$n \tcstares at $p and utters the words, '\tC%s\tc'.\tn";
    else
      format = "\tn$n \tcstares at $p and begins chanting the words, '\tC%s\tc'.\tn";
  } else {
    if (!start)
      format = "\tn$n \tcutters the words, '\tC%s\tc'.\tn";
    else
      format = "\tn$n \tcbegins chanting the words, '\tC%s\tc'.\tn";
  }

  snprintf(buf1, sizeof(buf1), format, skill_name(spellnum));
  snprintf(buf2, sizeof(buf2), format, buf);

  for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room) {
    if (i == ch || i == tch || !i->desc || !AWAKE(i))
      continue;

    if (!IS_NPC(i))
      attempt = compute_ability(i, ABILITY_SPELLCRAFT) + dice(1,20);
    else
      attempt = 10 + dice(1,20);

    if (attempt > dc_of_id)
      perform_act(buf1, ch, tobj, tch, i);
    else
      perform_act(buf2, ch, tobj, tch, i);
  }

  if (tch != NULL && !IS_NPC(tch))
    attempt = compute_ability(tch, ABILITY_SPELLCRAFT) + dice(1,20);
  else
    attempt = 10 + dice(1,20);
  
  if (tch != NULL && tch != ch && IN_ROOM(tch) == IN_ROOM(ch)) {
    if (!start)
      snprintf(buf1, sizeof(buf1), "\tn$n \tcstares at you and utters the words, '\tC%s\tc'.\tn",
	    attempt > dc_of_id ? skill_name(spellnum) : buf);
    else
      snprintf(buf1, sizeof(buf1),
"\tn$n \tcweaves $s hands in an intricate pattern and begins to chant the words, '\tC%s\tc' at you.\tn",
	    attempt > dc_of_id ? skill_name(spellnum) : buf);
    act(buf1, FALSE, ch, NULL, tch, TO_VICT);
  }

}


bool isEpicSpell(int spellnum)
{
  switch(spellnum) {
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
 * this because you can guarantee > 0 and <= TOP_SPELL_DEFINE. */
const char *skill_name(int num)
{
  if (num > 0 && num <= TOP_SPELL_DEFINE)
    return (spell_info[num].name);
  else if (num == -1)
    return ("UNUSED");
  else
    return ("Non-Spell-Effect");
}

int find_skill_num(char *name)
{
  int skindex, ok;
  char *temp, *temp2;
  char first[256], first2[256], tempbuf[256];

  for (skindex = 1; skindex <= TOP_SPELL_DEFINE; skindex++) {
    if (is_abbrev(name, spell_info[skindex].name))
      return (skindex);

    ok = TRUE;
    strlcpy(tempbuf, spell_info[skindex].name, sizeof(tempbuf));	/* strlcpy: OK */
    temp = any_one_arg(tempbuf, first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok) {
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

int find_ability_num(char *name)
{
  int skindex, ok;
  char *temp, *temp2;
  char first[256], first2[256], tempbuf[256];

  for (skindex = 1; skindex < NUM_ABILITIES; skindex++) {
    if (is_abbrev(name, ability_names[skindex]))
      return (skindex);

    ok = TRUE;
    strlcpy(tempbuf, ability_names[skindex], sizeof(tempbuf));	
    temp = any_one_arg(tempbuf, first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok) {
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
	     struct obj_data *ovict, int spellnum, int level, int casttype)
{
  int savetype;

  if (spellnum < 1 || spellnum > TOP_SPELL_DEFINE)
    return (0);

  if (!cast_wtrigger(caster, cvict, ovict, spellnum))
    return 0;
  if (!cast_otrigger(caster, ovict, spellnum))
    return 0;
  if (!cast_mtrigger(caster, cvict, spellnum))
    return 0;

  if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOMAGIC)) {
    send_to_char(caster, "Your magic fizzles out and dies.\r\n");
    act("$n's magic fizzles out and dies.", FALSE, caster, 0, 0, TO_ROOM);
    return (0);
  }
  if (cvict && ROOM_FLAGGED(IN_ROOM(cvict), ROOM_NOMAGIC)) {
    send_to_char(caster, "Your magic fizzles out and dies.\r\n");
    act("$n's magic fizzles out and dies.", FALSE, caster, 0, 0, TO_ROOM);
    return (0);
  }  
  if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_PEACEFUL) &&
      (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE))) {
    send_to_char(caster, "A flash of white light fills the room, dispelling your violent magic!\r\n");
    act("White light from no particular source suddenly fills the room, then vanishes.", FALSE, caster, 0, 0, TO_ROOM);
    return (0);
  }
  if (cvict && MOB_FLAGGED(cvict, MOB_NOKILL)) {
    send_to_char(caster, "This mob is protected.\r\n");
    return (0);
  }

  //attach event for epic spells, increase skill
  switch(spellnum) {
    case SPELL_MUMMY_DUST:
      attach_mud_event(new_mud_event(eMUMMYDUST, caster, NULL), 3 * SECS_PER_MUD_DAY);
      if (!IS_NPC(caster))
        increase_skill(caster, SKILL_MUMMY_DUST);
      break;
    case SPELL_DRAGON_KNIGHT:
      attach_mud_event(new_mud_event(eDRAGONKNIGHT, caster, NULL), 3 * SECS_PER_MUD_DAY);
      if (!IS_NPC(caster))
        increase_skill(caster, SKILL_DRAGON_KNIGHT);
      break;
    case SPELL_GREATER_RUIN:
      attach_mud_event(new_mud_event(eGREATERRUIN, caster, NULL), 3 * SECS_PER_MUD_DAY);
      if (!IS_NPC(caster))
        increase_skill(caster, SKILL_GREATER_RUIN);
      break;
    case SPELL_HELLBALL:
      attach_mud_event(new_mud_event(eHELLBALL, caster, NULL), 3 * SECS_PER_MUD_DAY);
      if (!IS_NPC(caster))
        increase_skill(caster, SKILL_HELLBALL);
      break;
    case SPELL_EPIC_MAGE_ARMOR:
      attach_mud_event(new_mud_event(eEPICMAGEARMOR, caster, NULL), 3 * SECS_PER_MUD_DAY);
      if (!IS_NPC(caster))
        increase_skill(caster, SKILL_EPIC_MAGE_ARMOR);
      break;
    case SPELL_EPIC_WARDING:
      attach_mud_event(new_mud_event(eEPICWARDING, caster, NULL), 3 * SECS_PER_MUD_DAY);
      if (!IS_NPC(caster))
        increase_skill(caster, SKILL_EPIC_WARDING);
      break;
  }
          
  /* determine the type of saving throw */
  switch (casttype) {
  case CAST_STAFF:
  case CAST_SCROLL:
  case CAST_POTION:
  case CAST_WAND:
    savetype = SAVING_WILL;
    break;
  case CAST_SPELL:
    savetype = SAVING_WILL;
    break;
  default:
    savetype = SAVING_WILL;
    break;
  }

  if (IS_SET(SINFO.routines, MAG_DAMAGE))
    if (mag_damage(level, caster, cvict, ovict, spellnum, savetype) == -1)
      return (-1);	/* Successful and target died, don't cast again. */

  if (IS_SET(SINFO.routines, MAG_AFFECTS))
    mag_affects(level, caster, cvict, ovict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_UNAFFECTS))
    mag_unaffects(level, caster, cvict, ovict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_POINTS))
    mag_points(level, caster, cvict, ovict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_ALTER_OBJS))
    mag_alter_objs(level, caster, ovict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_GROUPS))
    mag_groups(level, caster, ovict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_MASSES))
    mag_masses(level, caster, ovict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_AREAS))
    mag_areas(level, caster, ovict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_SUMMONS))
    mag_summons(level, caster, ovict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_CREATIONS))
    mag_creations(level, caster, ovict, spellnum);

  if (IS_SET(SINFO.routines, MAG_ROOM))
    mag_room(level, caster, ovict, spellnum);

  if (IS_SET(SINFO.routines, MAG_MANUAL))
    switch (spellnum) {
    case SPELL_CHARM:		MANUAL_SPELL(spell_charm); break;
    case SPELL_CREATE_WATER:	MANUAL_SPELL(spell_create_water); break;
    case SPELL_DETECT_POISON:	MANUAL_SPELL(spell_detect_poison); break;
    case SPELL_ENCHANT_WEAPON:  MANUAL_SPELL(spell_enchant_weapon); break;
    case SPELL_IDENTIFY:	MANUAL_SPELL(spell_identify); break;
    case SPELL_WIZARD_EYE:	MANUAL_SPELL(spell_wizard_eye); break;
    case SPELL_LOCATE_OBJECT:   MANUAL_SPELL(spell_locate_object); break;
    case SPELL_POLYMORPH:	MANUAL_SPELL(spell_polymorph); break;
    case SPELL_SUMMON:		MANUAL_SPELL(spell_summon); break;
    case SPELL_WORD_OF_RECALL:  MANUAL_SPELL(spell_recall); break;
    case SPELL_TELEPORT:	MANUAL_SPELL(spell_teleport); break;
    case SPELL_ACID_ARROW:	MANUAL_SPELL(spell_acid_arrow); break;
    case SPELL_CLAIRVOYANCE:	MANUAL_SPELL(spell_clairvoyance); break;
    case SPELL_DISPEL_MAGIC:	MANUAL_SPELL(spell_dispel_magic); break;
    }

    if (SINFO.violent && cvict && GET_POS(cvict) == POS_STANDING &&
	   spellnum != SPELL_CHARM)
      if (cvict != caster) {  // funny results from potions/scrolls
        if (IN_ROOM(cvict) == IN_ROOM(caster))
          hit(cvict, caster, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }

    return (1);
}

/* mag_objectmagic: This is the entry-point for all magic items.  This should
 * only be called by the 'quaff', 'use', 'recite', etc. routines.
 * For reference, object values 0-3:
 * staff  - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * wand   - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * scroll - [0]	level	[1] spell num	[2] spell num	[3] spell num
 * potion - [0] level	[1] spell num	[2] spell num	[3] spell num
 * Staves and wands will default to level 14 if the level is not specified; the
 * DikuMUD format did not specify staff and wand levels in the world files */
void mag_objectmagic(struct char_data *ch, struct obj_data *obj,
		          char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  int i, k;
  struct char_data *tch = NULL, *next_tch;
  struct obj_data *tobj = NULL;

  one_argument(argument, arg);

  k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM |
		   FIND_OBJ_EQUIP, ch, &tch, &tobj);

  switch (GET_OBJ_TYPE(obj)) {
  case ITEM_STAFF:
    act("You tap $p three times on the ground.", FALSE, ch, obj, 0, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, 0, TO_ROOM);
    else
      act("$n taps $p three times on the ground.", FALSE, ch, obj, 0, TO_ROOM);

    if (GET_OBJ_VAL(obj, 2) <= 0) {
      send_to_char(ch, "It seems powerless.\r\n");
      act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
    } else {
      GET_OBJ_VAL(obj, 2)--;
      WAIT_STATE(ch, PULSE_VIOLENCE);
      /* Level to cast spell at. */
      k = GET_OBJ_VAL(obj, 0) ? GET_OBJ_VAL(obj, 0) : DEFAULT_STAFF_LVL;

      /* Area/mass spells on staves can cause crashes. So we use special cases
       * for those spells spells here. */
      if (HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, 3), MAG_MASSES | MAG_AREAS)) {
        for (i = 0, tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
	  i++;
	while (i-- > 0)
	  call_magic(ch, NULL, NULL, GET_OBJ_VAL(obj, 3), k, CAST_STAFF);
      } else {
	for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
	  next_tch = tch->next_in_room;
	  if (ch != tch)
	    call_magic(ch, tch, NULL, GET_OBJ_VAL(obj, 3), k, CAST_STAFF);
	}
      }
    }
    break;
  case ITEM_WAND:
    if (k == FIND_CHAR_ROOM) {
      if (tch == ch) {
	act("You point $p at yourself.", FALSE, ch, obj, 0, TO_CHAR);
	act("$n points $p at $mself.", FALSE, ch, obj, 0, TO_ROOM);
      } else {
	act("You point $p at $N.", FALSE, ch, obj, tch, TO_CHAR);
	if (obj->action_description)
	  act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
	else
	  act("$n points $p at $N.", TRUE, ch, obj, tch, TO_ROOM);
      }
    } else if (tobj != NULL) {
      act("You point $p at $P.", FALSE, ch, obj, tobj, TO_CHAR);
      if (obj->action_description)
	act(obj->action_description, FALSE, ch, obj, tobj, TO_ROOM);
      else
	act("$n points $p at $P.", TRUE, ch, obj, tobj, TO_ROOM);
    } else if (IS_SET(spell_info[GET_OBJ_VAL(obj, 3)].routines, MAG_AREAS | MAG_MASSES)) {
      /* Wands with area spells don't need to be pointed. */
      act("You point $p outward.", FALSE, ch, obj, NULL, TO_CHAR);
      act("$n points $p outward.", TRUE, ch, obj, NULL, TO_ROOM);
    } else {
      act("At what should $p be pointed?", FALSE, ch, obj, NULL, TO_CHAR);
      return;
    }

    if (GET_OBJ_VAL(obj, 2) <= 0) {
      send_to_char(ch, "It seems powerless.\r\n");
      act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
      return;
    }
    GET_OBJ_VAL(obj, 2)--;
    WAIT_STATE(ch, PULSE_VIOLENCE);
    if (GET_OBJ_VAL(obj, 0))
      call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3),
		 GET_OBJ_VAL(obj, 0), CAST_WAND);
    else
      call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3),
		 DEFAULT_WAND_LVL, CAST_WAND);
    break;
  case ITEM_SCROLL:
    if (*arg) {
      if (!k) {
	act("There is nothing to here to affect with $p.", FALSE,
	    ch, obj, NULL, TO_CHAR);
	return;
      }
    } else
      tch = ch;

    act("You recite $p which dissolves.", TRUE, ch, obj, 0, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
    else
      act("$n recites $p.", FALSE, ch, obj, NULL, TO_ROOM);

    WAIT_STATE(ch, PULSE_VIOLENCE);
    for (i = 1; i <= 3; i++)
      if (call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, i),
		       GET_OBJ_VAL(obj, 0), CAST_SCROLL) <= 0)
	break;

    if (obj != NULL)
      extract_obj(obj);
    break;
  case ITEM_POTION:
    tch = ch;

  if (!consume_otrigger(obj, ch, OCMD_QUAFF))  /* check trigger */
    return;

    act("You quaff $p.", FALSE, ch, obj, NULL, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
    else
      act("$n quaffs $p.", TRUE, ch, obj, NULL, TO_ROOM);

    WAIT_STATE(ch, PULSE_VIOLENCE);
    for (i = 1; i <= 3; i++)
      if (call_magic(ch, ch, NULL, GET_OBJ_VAL(obj, i),
		       GET_OBJ_VAL(obj, 0), CAST_POTION) <= 0)
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
  IS_CASTING(ch) = 		FALSE;
  CASTING_TIME(ch) = 		0;
  CASTING_SPELLNUM(ch) =	0;
  CASTING_TCH(ch) =		NULL;
  CASTING_TOBJ(ch) =		NULL;
}

int castingCheckOk(struct char_data *ch)
{
  int spellnum = CASTING_SPELLNUM(ch);

  if ((GET_POS(ch) != POS_STANDING &&
	GET_POS(ch) != POS_FIGHTING) ||
        (CASTING_TOBJ(ch) && CASTING_TOBJ(ch)->in_room != ch->in_room &&
		!IS_SET(SINFO.targets, TAR_OBJ_WORLD | TAR_OBJ_INV)) ||
        (CASTING_TCH(ch) && CASTING_TCH(ch)->in_room != ch->in_room && SINFO.violent)) {
    act("A spell from $n is aborted!", FALSE, ch, 0, 0,
                TO_ROOM);
    send_to_char(ch, "You are unable to continue your spell!\r\n");
    resetCastingData(ch);
    return 0;
  }
  if (AFF_FLAGGED(ch, AFF_NAUSEATED)) {
    send_to_char(ch, "You are too nauseated to continue casting!\r\n");
    act("$n seems to be too nauseated to continue casting!",
            TRUE, ch, 0, 0, TO_ROOM);
    resetCastingData(ch);
    return (0);
  }  
  return 1;
}


void finishCasting(struct char_data *ch)
{
  say_spell(ch, CASTING_SPELLNUM(ch), CASTING_TCH(ch), CASTING_TOBJ(ch), FALSE);
  send_to_char(ch, "You complete your spell...");   
  call_magic(ch, CASTING_TCH(ch), CASTING_TOBJ(ch), CASTING_SPELLNUM(ch),
	CASTER_LEVEL(ch), CAST_SPELL);
  resetCastingData(ch);
}


EVENTFUNC(event_casting)
{
  struct char_data *ch;
  struct mud_event_data *pMudEvent;
  int x, failure = -1;
  char buf[MAX_INPUT_LENGTH];

  //initialize everything and dummy checks
  if (event_obj == NULL) return 0;
  pMudEvent = (struct mud_event_data *) event_obj;
  ch = (struct char_data *) pMudEvent->pStruct;
  if (!IS_NPC(ch) && !IS_PLAYING(ch->desc)) return 0;
  int spellnum = CASTING_SPELLNUM(ch);

  // is he casting?
  if (!IS_CASTING(ch))
    return 0;
    
  // still some time left to cast
  if (CASTING_TIME(ch) > 0) {

    //checking positions, targets
    if (!castingCheckOk(ch))
      return 0;
    else {
	// concentration challenge
      failure += spell_info[spellnum].min_level[CASTING_CLASS(ch)] * 2;
      if (!IS_NPC(ch))
        failure -= CASTER_LEVEL(ch) + ((GET_ABILITY(ch, ABILITY_CONCENTRATION) - 3) * 2);
      else
        failure -= (GET_LEVEL(ch)) * 2;
        //chance of failure calculated here, so far:  taunt, grappled
      if (char_has_mud_event(ch, eTAUNTED))
        failure += 10;
      if (AFF_FLAGGED(ch, AFF_GRAPPLED))
        failure += 10;

      if (dice(1,101) < failure) {
        send_to_char(ch, "You lost your concentration!\r\n");
        resetCastingData(ch);
        return 0;
      }
  
      //display time left to finish spell
      sprintf(buf, "Casting: %s ", SINFO.name);
      for (x = CASTING_TIME(ch); x > 0; x--)
        strcat(buf, "*");
      strcat(buf, "\r\n");
      send_to_char(ch, buf);

      if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_QUICK_CHANT))
        if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_QUICK_CHANT) > dice(1, 100))
          CASTING_TIME(ch)--;
          
      CASTING_TIME(ch)--;

      //chance quick chant bumped us to finish early
      if (CASTING_TIME(ch) <= 0) {

        //do all our checks
        if (!castingCheckOk(ch))
          return 0;
        
        finishCasting(ch);
        return 0;
      } else
        return 1 * PASSES_PER_SEC;
    }

  //spell needs to be completed now (casting time <= 0)
  } else {

    //do all our checks
    if (!castingCheckOk(ch))
      return 0;
    else {
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
	           struct obj_data *tobj, int spellnum)
{
  if (spellnum < 0 || spellnum > TOP_SPELL_DEFINE) {
    log("SYSERR: cast_spell trying to call spellnum %d/%d.", spellnum,
	TOP_SPELL_DEFINE);
    return (0);
  }

  if (GET_POS(ch) < SINFO.min_position) {
    switch (GET_POS(ch)) {
      case POS_SLEEPING:
      send_to_char(ch, "You dream about great magical powers.\r\n");
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
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == tch)) {
    send_to_char(ch, "You are afraid you might hurt your master!\r\n");
    return (0);
  }
  if ((tch != ch) && IS_SET(SINFO.targets, TAR_SELF_ONLY)) {
    send_to_char(ch, "You can only cast this spell upon yourself!\r\n");
    return (0);
  }
  if ((tch == ch) && IS_SET(SINFO.targets, TAR_NOT_SELF)) {
    send_to_char(ch, "You cannot cast this spell upon yourself!\r\n");
    return (0);
  }
  if (IS_SET(SINFO.routines, MAG_GROUPS) && !GROUP(ch)) {
    send_to_char(ch, "You can't cast this spell if you're not in a group!\r\n");
    return (0);
  }
  if (AFF_FLAGGED(ch, AFF_NAUSEATED)) {
    send_to_char(ch, "You are too nauseated to cast!\r\n");
    act("$n seems to be too nauseated to cast!",
            TRUE, ch, 0, 0, TO_ROOM);
    return (0);
  }
  

  //default casting class will be the highest level casting class
  int class = -1, clevel = -1;

  if (IS_MAGIC_USER(ch)) {
    class = CLASS_MAGIC_USER;
    clevel = IS_MAGIC_USER(ch);
  }
  if (IS_CLERIC(ch) > clevel) {
    class = CLASS_CLERIC;
    clevel = IS_CLERIC(ch);
  }
  if (IS_DRUID(ch) > clevel) {
    class = CLASS_DRUID;
    clevel = IS_DRUID(ch);
  }

  if (!isEpicSpell(spellnum) && !IS_NPC(ch) &&
      spellnum != SPELL_ACID_SPLASH && spellnum != SPELL_RAY_OF_FROST) {
    class = forgetSpell(ch, spellnum, -1);
    if (class == -1) {
      send_to_char(ch, "ERR:  Report BUG98237 to an IMM!\r\n");
      return 0;
    }

    addSpellMemming(ch, spellnum, spell_info[spellnum].memtime, class);
  }

  if (SINFO.time <= 0) {
    send_to_char(ch, "%s", CONFIG_OK);
    say_spell(ch, spellnum, tch, tobj, FALSE);
    return (call_magic(ch, tch, tobj, spellnum, CASTER_LEVEL(ch), CAST_SPELL));
  }

  //casting time entry point
  if (char_has_mud_event(ch, eCASTING)) {
    send_to_char(ch, "You are already attempting to cast!\r\n");
    return (0);
  }
  send_to_char(ch, "You begin casting your spell...\r\n");
  say_spell(ch, spellnum, tch, tobj, TRUE);
  IS_CASTING(ch) = TRUE;
  CASTING_TIME(ch) = SINFO.time;
  CASTING_TCH(ch) = tch;
  CASTING_TOBJ(ch) = tobj;
  CASTING_SPELLNUM(ch) = spellnum;
  CASTING_CLASS(ch) = class;
  NEW_EVENT(eCASTING, ch, NULL, 1 * PASSES_PER_SEC);

  //this return value has to be checked -zusuk
  return (1);
}

ACMD(do_abort)
{
  if (IS_NPC(ch))
    return;

  if (!IS_CASTING(ch)) {
    send_to_char(ch, "You aren't casting!\r\n");
    return;
  }
 
  send_to_char(ch, "You abort your spell.\r\n");
  resetCastingData(ch);
}

/* do_cast is the entry point for PC-casted spells.  It parses the arguments,
 * determines the spell number and finds a target, throws the die to see if
 * the spell can be cast, checks for sufficient mana and subtracts it, and
 * passes control to cast_spell(). */
ACMD(do_cast)
{
  struct char_data *tch = NULL;
  struct obj_data *tobj = NULL;
  char *s, *t;
  int number, spellnum, i, target = 0;
  // int mana;

  if (IS_NPC(ch))
    return;

  /* get: blank, spell name, target name */
  s = strtok(argument, "'");

  if (s == NULL) {
    send_to_char(ch, "Cast what where?\r\n");
    return;
  }
  s = strtok(NULL, "'");
  if (s == NULL) {
    send_to_char(ch, "Spell names must be enclosed in the Holy Magic Symbols: '\r\n");
    return;
  }
  t = strtok(NULL, "\0");

  skip_spaces(&s);

  /* spellnum = search_block(s, spells, 0); */
  spellnum = find_skill_num(s);

  if ((spellnum < 1) || (spellnum > MAX_SPELLS) || !*s) {
    send_to_char(ch, "Cast what?!?\r\n");
    return;
  }

  if (!IS_CASTER(ch)) {
    send_to_char(ch, "You are not even a caster!\r\n");
    return;
  }

  if (CLASS_LEVEL(ch, CLASS_MAGIC_USER) < SINFO.min_level[CLASS_MAGIC_USER] &&
	CLASS_LEVEL(ch, CLASS_CLERIC) < SINFO.min_level[CLASS_CLERIC] &&
	CLASS_LEVEL(ch, CLASS_DRUID) < SINFO.min_level[CLASS_DRUID]
  ) {
    send_to_char(ch, "You do not know that spell!\r\n");
    return;
  }

  if (GET_SKILL(ch, spellnum) == 0) {
    send_to_char(ch, "You are unfamiliar with that spell.\r\n");
    return;
  }

  if (!hasSpell(ch, spellnum) && !isEpicSpell(spellnum)
          && spellnum != SPELL_ACID_SPLASH && spellnum != SPELL_RAY_OF_FROST) {
    send_to_char(ch, "You do not seem to have that spell prepared... (help memorization)\r\n");
    return;
  }

  if (CLASS_LEVEL(ch, CLASS_MAGIC_USER) && GET_INT(ch) < 10) {
    send_to_char(ch, "You are not smart enough to cast spells...\r\n");
    return;
  }
  if (CLASS_LEVEL(ch, CLASS_CLERIC) && GET_WIS(ch) < 10) {
    send_to_char(ch, "You are not wise enough to cast spells...\r\n");
    return;
  }
  if (CLASS_LEVEL(ch, CLASS_DRUID) && GET_WIS(ch) < 10) {
    send_to_char(ch, "You are not wise enough to cast spells...\r\n");
    return;
  }

  //epic spell cooldown
  if (char_has_mud_event(ch, eMUMMYDUST) && spellnum == SPELL_MUMMY_DUST) {
    send_to_char(ch, "You must wait longer before you can use this spell again.\r\n");
    send_to_char(ch, "OOC:  The cooldown is approximately 9 minutes.\r\n");
    return;
  }
  if (char_has_mud_event(ch, eDRAGONKNIGHT) && spellnum == SPELL_DRAGON_KNIGHT) {
    send_to_char(ch, "You must wait longer before you can use this spell again.\r\n");
    send_to_char(ch, "OOC:  The cooldown is approximately 9 minutes.\r\n");
    return;
  }
  if (char_has_mud_event(ch, eGREATERRUIN) && spellnum == SPELL_GREATER_RUIN) {
    send_to_char(ch, "You must wait longer before you can use this spell again.\r\n");
    send_to_char(ch, "OOC:  The cooldown is approximately 9 minutes.\r\n");
    return;
  }
  if (char_has_mud_event(ch, eHELLBALL) && spellnum == SPELL_HELLBALL) {
    send_to_char(ch, "You must wait longer before you can use this spell again.\r\n");
    send_to_char(ch, "OOC:  The cooldown is approximately 9 minutes.\r\n");
    return;
  }
  if (char_has_mud_event(ch, eEPICMAGEARMOR) && spellnum == SPELL_EPIC_MAGE_ARMOR) {
    send_to_char(ch, "You must wait longer before you can use this spell again.\r\n");
    send_to_char(ch, "OOC:  The cooldown is approximately 9 minutes.\r\n");
    return;
  }
  if (char_has_mud_event(ch, eEPICWARDING) && spellnum == SPELL_EPIC_WARDING) {
    send_to_char(ch, "You must wait longer before you can use this spell again.\r\n");
    send_to_char(ch, "OOC:  The cooldown is approximately 9 minutes.\r\n");
    return;
  }

  /* Find the target */
  if (t != NULL) {
    char arg[MAX_INPUT_LENGTH];

    strlcpy(arg, t, sizeof(arg));
    one_argument(arg, t);
    skip_spaces(&t);

    /* Copy target to global cast_arg2, for use in spells like locate object */
    strcpy(cast_arg2, t);
  }
  if (IS_SET(SINFO.targets, TAR_IGNORE)) {
    target = TRUE;
  } else if (t != NULL && *t) {
    number = get_number(&t);
    if (!target && (IS_SET(SINFO.targets, TAR_CHAR_ROOM))) {
      if ((tch = get_char_vis(ch, t, &number, FIND_CHAR_ROOM)) != NULL)
	target = TRUE;
    }
    if (!target && IS_SET(SINFO.targets, TAR_CHAR_WORLD))
      if ((tch = get_char_vis(ch, t, &number, FIND_CHAR_WORLD)) != NULL)
	target = TRUE;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_INV))
      if ((tobj = get_obj_in_list_vis(ch, t, &number, ch->carrying)) != NULL)
	target = TRUE;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_EQUIP)) {
      for (i = 0; !target && i < NUM_WEARS; i++)
	if (GET_EQ(ch, i) && isname(t, GET_EQ(ch, i)->name)) {
	  tobj = GET_EQ(ch, i);
	  target = TRUE;
	}
    }
    if (!target && IS_SET(SINFO.targets, TAR_OBJ_ROOM))
      if ((tobj = get_obj_in_list_vis(ch, t, &number, world[IN_ROOM(ch)].contents)) != NULL)
	target = TRUE;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_WORLD))
      if ((tobj = get_obj_vis(ch, t, &number)) != NULL)
	target = TRUE;

  } else {			/* if target string is empty */
    if (!target && IS_SET(SINFO.targets, TAR_FIGHT_SELF))
      if (FIGHTING(ch) != NULL) {
	tch = ch;
	target = TRUE;
      }
    if (!target && IS_SET(SINFO.targets, TAR_FIGHT_VICT))
      if (FIGHTING(ch) != NULL) {
	tch = FIGHTING(ch);
	target = TRUE;
      }
    /* if no target specified, and the spell isn't violent, default to self */
    if (!target && IS_SET(SINFO.targets, TAR_CHAR_ROOM) &&
	!SINFO.violent) {
      tch = ch;
      target = TRUE;
    }
    if (!target) {
      send_to_char(ch, "Upon %s should the spell be cast?\r\n",
		IS_SET(SINFO.targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP) ? "what" : "who");
      return;
    }
  }

  if (target && (tch == ch) && SINFO.violent && (spellnum != SPELL_DISPEL_MAGIC)) {
    send_to_char(ch, "You shouldn't cast that on yourself -- could be bad for your health!\r\n");
    return;
  }
  if (!target) {
    send_to_char(ch, "Cannot find the target of your spell!\r\n");
    return;
  }

  //maybe use this as a way to keep npc's in check
//  mana = mag_manacost(ch, spellnum);
//  if ((mana > 0) && (GET_MANA(ch) < mana) && (GET_LEVEL(ch) < LVL_IMMORT)) {
//    send_to_char(ch, "You haven't the energy to cast that spell!\r\n");
//    return;
//  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC)) {
    send_to_char(ch, "Your magic fizzles out and dies.\r\n");
    act("$n's magic fizzles out and dies.", FALSE, ch, 0, 0, TO_ROOM);
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) &&
      (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE))) {
    send_to_char(ch, "A flash of white light fills the room, dispelling your violent magic!\r\n");
    act("White light from no particular source suddenly fills the room, then vanishes.",FALSE, ch, 0, 0, TO_ROOM);
    return;
  }

  if (cast_spell(ch, tch, tobj, spellnum)) {
    WAIT_STATE(ch, PULSE_VIOLENCE);
  // maybe use this as a way to keep npc's in check
  //   if (mana > 0)
  //     GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
  }
}

void spell_level(int spell, int chclass, int level)
{
  int bad = 0;

  if (spell < 0 || spell > TOP_SPELL_DEFINE) {
    log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, TOP_SPELL_DEFINE);
    return;
  }

  if (chclass < 0 || chclass >= NUM_CLASSES) {
    log("SYSERR: assigning '%s' to illegal class %d/%d.", skill_name(spell),
		chclass, NUM_CLASSES - 1);
    bad = 1;
  }

  if (level < 1 || level > LVL_IMPL) {
    log("SYSERR: assigning '%s' to illegal level %d/%d.", skill_name(spell),
		level, LVL_IMPL);
    bad = 1;
  }

  if (!bad)
    spell_info[spell].min_level[chclass] = level;
}


/* Assign the spells on boot up */
static void spello(int spl, const char *name, int max_mana, int min_mana,
		int mana_change, int minpos, int targets, int violent,
		int routines, const char *wearoff, int time, int memtime, int school)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++)
    spell_info[spl].min_level[i] = LVL_IMMORT;
  spell_info[spl].mana_max = max_mana;
  spell_info[spl].mana_min = min_mana;
  spell_info[spl].mana_change = mana_change;
  spell_info[spl].min_position = minpos;
  spell_info[spl].targets = targets;
  spell_info[spl].violent = violent;
  spell_info[spl].routines = routines;
  spell_info[spl].name = name;
  spell_info[spl].wear_off_msg = wearoff;
  spell_info[spl].time = time;
  spell_info[spl].memtime = memtime;
  spell_info[spl].schoolOfMagic = school;
}

void unused_spell(int spl)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++)
    spell_info[spl].min_level[i] = LVL_IMPL + 1;
  spell_info[spl].mana_max = 0;
  spell_info[spl].mana_min = 0;
  spell_info[spl].mana_change = 0;
  spell_info[spl].min_position = 0;
  spell_info[spl].targets = 0;
  spell_info[spl].violent = 0;
  spell_info[spl].routines = 0;
  spell_info[spl].name = unused_spellname;
  spell_info[spl].wear_off_msg = unused_wearoff;
  spell_info[spl].time = 0;
  spell_info[spl].memtime = 0;
  spell_info[spl].schoolOfMagic = 0; // noschool
}


#define skillo(skill, name) spello(skill, name, 0, 0, 0, 0, 0, 0, 0, NULL, 0, 0, 0);


/* Arguments for spello calls:
 * spellnum, maxmana, minmana, manachng, minpos, targets, violent?, routines.
 * spellnum:  Number of the spell.  Usually the symbolic name as defined in
 *  spells.h (such as SPELL_HEAL).
 * spellname: The name of the spell.
 * maxmana :  The maximum mana this spell will take (i.e., the mana it
 *  will take when the player first gets the spell).
 * minmana :  The minimum mana this spell will take, no matter how high
 *  level the caster is.
 * manachng:  The change in mana for the spell from level to level.  This
 *  number should be positive, but represents the reduction in mana cost as
 *  the caster's level increases.
 * minpos  :  Minimum position the caster must be in for the spell to work
 *  (usually fighting or standing). targets :  A "list" of the valid targets
 *  for the spell, joined with bitwise OR ('|').
 * violent :  TRUE or FALSE, depending on if this is considered a violent
 *  spell and should not be cast in PEACEFUL rooms or on yourself.  Should be
 *  set on any spell that inflicts damage, is considered aggressive (i.e.
 *  charm, curse), or is otherwise nasty.
 * routines:  A list of magic routines which are associated with this spell
 *  if the spell uses spell templates.  Also joined with bitwise OR ('|').
 * time:  casting time of the spell 
 * memtime:  memtime of the spell 
 * See the documentation for a more detailed description of these fields. You
 * only need a spello() call to define a new spell; to decide who gets to use
 * a spell or skill, look in class.c.  -JE */

 /* leave these here for my usage -zusuk */
			/* evocation */
                        /* conjuration */
			/* necromancy */
			/* enchantment */
			/* illusion */
			/* divination */
			/* abjuration */
			/* transmutation */

void mag_assign_spells(void)
{
  int i;

  /* Do not change the loop below. */
  for (i = 0; i <= TOP_SPELL_DEFINE; i++)
    unused_spell(i);
  /* Do not change the loop above. */

  // sorted the spells by shared / magical / divine, and by circle
  // in each category -zusuk

  //shared
  spello(SPELL_INFRAVISION, "infravision", 44, 29, 1, POS_FIGHTING,  //enchant
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your night vision seems to fade.", 4, 8,
	ENCHANTMENT);  // mage 4, cleric 4
  spello(SPELL_DETECT_POISON, "detect poison", 72, 57, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL,
	"The detect poison wears off.", 4, 8,
	DIVINATION); // mage 7, cleric 2
  spello(SPELL_POISON, "poison", 85, 70, 1, POS_FIGHTING,  //enchantment
	TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV, TRUE,
	MAG_AFFECTS | MAG_ALTER_OBJS,
	"You feel less sick.", 5, 8,
	ENCHANTMENT);  // mage 4, cleric 5
  spello(SPELL_ENERGY_DRAIN, "energy drain", 79, 64, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_MANUAL,
	NULL, 9, 14,
	NECROMANCY);  // mage 8, cleric 9
  spello(SPELL_REMOVE_CURSE, "remove curse", 79, 64, 1, POS_FIGHTING,  //abjur
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE,
	MAG_UNAFFECTS | MAG_ALTER_OBJS,
	NULL, 4, 8, ABJURATION);  // mage 4

  //shared epic
  spello(SPELL_DRAGON_KNIGHT, "dragon knight", 95, 80, 1, POS_FIGHTING,
	TAR_IGNORE, FALSE, MAG_SUMMONS,
	NULL, 12, 1,
	NOSCHOOL);
  spello(SPELL_GREATER_RUIN, "greater ruin", 95, 80, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 6, 1,
	NOSCHOOL);
  spello(SPELL_HELLBALL, "hellball", 95, 80, 1, POS_FIGHTING,
	TAR_IGNORE, TRUE, MAG_AREAS,
	NULL, 6, 1,
	NOSCHOOL);
  spello(SPELL_MUMMY_DUST, "mummy dust", 95, 80, 1, POS_FIGHTING,
	TAR_IGNORE, FALSE, MAG_SUMMONS,
	NULL, 14, 1,
	NOSCHOOL);

  // magical

/* = =  cantrips  = = */
			/* evocation */
  spello(SPELL_ACID_SPLASH, "acid splash", 0, 0, 0, POS_SITTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 0, 1, EVOCATION);
  spello(SPELL_RAY_OF_FROST, "ray of frost", 0, 0, 0, POS_SITTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 0, 1, EVOCATION);
  
/* = =  1st circle  = = */
			/* evocation */
  spello(SPELL_MAGIC_MISSILE, "magic missile", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 0, 5, EVOCATION);
  spello(SPELL_HORIZIKAULS_BOOM, "horizikauls boom", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_AFFECTS,
	NULL, 1, 5, EVOCATION);
  spello(SPELL_BURNING_HANDS, "burning hands", 37, 22, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 1, 5, EVOCATION);
			/* conjuration */
  spello(SPELL_ICE_DAGGER, "ice dagger", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 1, 5, CONJURATION);
  spello(SPELL_MAGE_ARMOR, "mage armor", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel less protected.", 4, 5,
	CONJURATION);
  spello(SPELL_SUMMON_CREATURE_1, "summon creature i", 95, 80, 1, POS_FIGHTING,
	TAR_IGNORE, FALSE, MAG_SUMMONS,
	NULL, 5, 5, CONJURATION);
			/* necromancy */
  spello(SPELL_CHILL_TOUCH, "chill touch", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_AFFECTS,
	"You feel your strength return.", 1, 5, NECROMANCY);
  spello(SPELL_NEGATIVE_ENERGY_RAY, "negative energy ray", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 1, 5, NECROMANCY);
  spello(SPELL_RAY_OF_ENFEEBLEMENT, "ray of enfeeblement", 80, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
	"You feel your strength return.", 1, 5, NECROMANCY);
			/* enchantment */
  spello(SPELL_CHARM, "charm person", 51, 36, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_MANUAL,
	"You feel more self-confident.", 4, 5, ENCHANTMENT);
  spello(SPELL_ENCHANT_WEAPON, "enchant weapon", 44, 29, 1, POS_FIGHTING,
	TAR_OBJ_INV, FALSE, MAG_MANUAL,
	NULL, 5, 5, ENCHANTMENT);
  spello(SPELL_SLEEP, "sleep", 58, 43, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
	"You feel less tired.", 4, 5, ENCHANTMENT);
			/* illusion */
  spello(SPELL_COLOR_SPRAY, "color spray", 51, 36, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_AFFECTS,
	NULL, 1, 5, ILLUSION);
  spello(SPELL_SCARE, "scare", 80, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
	"You no longer feel scared.", 1, 5, ILLUSION);
  spello(SPELL_TRUE_STRIKE, "true strike", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel you are no longer able to strike true!", 0, 5, ILLUSION);
			/* divination */
  spello(SPELL_IDENTIFY, "identify", 85, 70, 1, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL,
        NULL, 5, 5, DIVINATION);
  spello(SPELL_SHELGARNS_BLADE, "shelgarns blade", 95, 80, 1, POS_FIGHTING,
	TAR_IGNORE, FALSE, MAG_SUMMONS,
	NULL, 6, 5, DIVINATION);
  spello(SPELL_GREASE, "grease", 80, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
	"You feel the grease spell wear off.", 1, 5, DIVINATION);
			/* abjuration */
  spello(SPELL_ENDURE_ELEMENTS, "endure elements", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel your element protection wear off.", 2, 5, ABJURATION);
  spello(SPELL_PROT_FROM_EVIL, "protection from evil", 58, 43, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel less protected from evil.", 5, 5, ABJURATION);
  spello(SPELL_PROT_FROM_GOOD, "protection from good", 58, 43, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel less protected from good.", 5, 5, ABJURATION);
			/* transmutation */
  spello(SPELL_EXPEDITIOUS_RETREAT, "expeditious retreat", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel less expeditious.", 0, 5,
	TRANSMUTATION);
  spello(SPELL_IRON_GUTS, "iron guts", 44, 29, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your guts feel less resillient.", 3, 5, TRANSMUTATION);
  spello(SPELL_SHIELD, "shield", 44, 29, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your magical shield fades away.", 2, 5, TRANSMUTATION);


/* = =  2nd circle  = = */
			/* evocation */
  spello(SPELL_SHOCKING_GRASP, "shocking grasp", 37, 22, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 2, 6, EVOCATION);
  spello(SPELL_SCORCHING_RAY, "scorching ray", 37, 22, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 2, 6, EVOCATION);
  spello(SPELL_CONTINUAL_FLAME, "continual flame", 58, 43, 1, POS_FIGHTING,
	TAR_IGNORE, FALSE, MAG_CREATIONS,
	NULL, 5, 6, EVOCATION);
                        /* conjuration */
  spello(SPELL_SUMMON_CREATURE_2, "summon creature ii", 95, 80, 1, POS_FIGHTING,
	TAR_IGNORE, FALSE, MAG_SUMMONS,
	NULL, 6, 6, CONJURATION);
  spello(SPELL_WEB, "web", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_AFFECTS,
	"You feel the sticky strands of the magical web dissolve.", 2, 6,
	CONJURATION);
  spello(SPELL_ACID_ARROW, "acid arrow", 37, 22, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_MANUAL,
	NULL, 2, 6, EVOCATION);
			/* necromancy */
  spello(SPELL_BLINDNESS, "blindness", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
	"You feel a cloak of blindness dissolve.", 3, 6,
	NECROMANCY); // cleric spell
  spello(SPELL_DEAFNESS, "deafness", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
	"You feel like you can hear again.", 3, 6,
	NECROMANCY);
  spello(SPELL_FALSE_LIFE, "false life", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel your necromantic-life drain away.", 4, 6, ILLUSION);
			/* enchantment */
  spello(SPELL_DAZE_MONSTER, "daze monster", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
	"You no longer feel dazed.", 2, 6,
	ENCHANTMENT);
  spello(SPELL_HIDEOUS_LAUGHTER, "hideous laughter", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
	"You feel able to control your laughter again.", 2, 6,
	ENCHANTMENT);
  spello(SPELL_TOUCH_OF_IDIOCY, "touch of idiocy", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
	"You begin to feel less incompetent.", 2, 6,
	ENCHANTMENT);
			/* illusion */
  spello(SPELL_BLUR, "blur", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel your blur spell wear off.", 2, 6, ILLUSION);
  spello(SPELL_INVISIBLE, "invisibility", 58, 43, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_AFFECTS | MAG_ALTER_OBJS,
	"You feel yourself exposed.", 4, 6, ILLUSION);
  spello(SPELL_MIRROR_IMAGE, "mirror image", 44, 29, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You watch as your images vanish.", 3, 6, ILLUSION);
			/* divination */
  spello(SPELL_DETECT_INVIS, "detect invisibility", 37, 22, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your eyes stop tingling.", 1, 6, DIVINATION);
  spello(SPELL_DETECT_MAGIC, "detect magic", 37, 22, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"The detect magic wears off.", 1, 6, DIVINATION);
  spello(SPELL_DARKNESS, "darkness", 50, 25, 5, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_ROOM, 
	"The cloak of darkness in the area dissolves.", 5, 6, DIVINATION);
			/* abjuration */
  spello(SPELL_RESIST_ENERGY, "resist energy", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your energy resistance dissipates.", 2, 6,
	ABJURATION);  // mage 1, cleric 1
  spello(SPELL_ENERGY_SPHERE, "energy sphere", 37, 22, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 2, 6, ABJURATION);
			/* transmutation */
  spello(SPELL_ENDURANCE, "endurance", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your magical endurance has faded away.", 2, 6,
	TRANSMUTATION);  // mage 1, cleric 1
  spello(SPELL_STRENGTH, "strength", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel weaker.", 2, 6, TRANSMUTATION);
  spello(SPELL_GRACE, "grace", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel less dextrous.", 2, 6, TRANSMUTATION);


  // 3rd cricle
			/* evocation */
  spello(SPELL_LIGHTNING_BOLT, "lightning bolt", 44, 29, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 3, 7, EVOCATION);
  spello(SPELL_FIREBALL, "fireball", 44, 29, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 3, 7, EVOCATION);
  spello(SPELL_WATER_BREATHE, "water breathe", 79, 64, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your magical gears fade away.", 7, 7, EVOCATION);
               /* conjuration */
  spello(SPELL_SUMMON_CREATURE_3, "summon creature iii", 95, 80, 1, POS_FIGHTING,
	TAR_IGNORE, FALSE, MAG_SUMMONS,
	NULL, 7, 7, CONJURATION);
  spello(SPELL_PHANTOM_STEED, "phantom steed", 95, 80, 1, POS_FIGHTING,
	TAR_IGNORE, FALSE, MAG_SUMMONS,
	NULL, 7, 7, CONJURATION);
  spello(SPELL_STINKING_CLOUD, "stinking cloud", 65, 50, 1, POS_FIGHTING,
	TAR_IGNORE, FALSE, MAG_ROOM,
	"You watch as the noxious gasses fade away.", 4, 7,
	CONJURATION);  
  spello(SPELL_STENCH, "stench", 65, 50, 1, POS_DEAD,
	TAR_IGNORE, FALSE, MAG_MASSES,
	"Your nausea from the noxious gas passes.", 4, 7,
	CONJURATION);  
			/* necromancy */
  spello(SPELL_HALT_UNDEAD, "halt undead", 65, 50, 1, POS_FIGHTING,
	TAR_IGNORE, TRUE, MAG_AREAS,
	"You feel the necromantic halt spell fade away.", 5, 7,
	NECROMANCY);
  spello(SPELL_VAMPIRIC_TOUCH, "vampiric touch", 44, 29, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_POINTS,
	NULL, 3, 7, NECROMANCY);
  spello(SPELL_HEROISM, "deathly heroism", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your deathly heroism fades away.", 4, 7,
	NECROMANCY);
			/* enchantment */
  spello(SPELL_FLY, "fly", 37, 22, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You drift slowly to the ground.", 3, 7, ENCHANTMENT);
  spello(SPELL_HOLD_PERSON, "hold person", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
	"You feel able to control your laughter again.", 3, 7,
	ENCHANTMENT);
  spello(SPELL_DEEP_SLUMBER, "deep slumber", 58, 43, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
	"You feel less tired.", 4, 7, ENCHANTMENT);
			/* illusion */
  spello(SPELL_WALL_OF_FOG, "wall of fog", 50, 25, 5, POS_FIGHTING,
	TAR_IGNORE, FALSE, MAG_ROOM, 
	"The wall of fog blows away.", 6, 7, ILLUSION);
  spello(SPELL_INVISIBILITY_SPHERE, "invisibility sphere", 58, 43, 1,
     POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS,
	NULL, 7, 7, ILLUSION);
  spello(SPELL_DAYLIGHT, "daylight", 50, 25, 5, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_ROOM, 
	"The artificial daylight fades away.", 6, 7, ILLUSION);
			/* divination */
  spello(SPELL_CLAIRVOYANCE, "clairvoyance", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL,
	NULL, 5, 7,
	DIVINATION);
  spello(SPELL_NON_DETECTION, "nondetection", 37, 22, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your non-detection spell wore off.", 6, 7, DIVINATION);
  spello(SPELL_DISPEL_MAGIC, "dispel magic", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_MANUAL,
	NULL, 4, 7, DIVINATION);
			/* abjuration */
  spello(SPELL_HASTE, "haste", 37, 22, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel your haste spell wear off.", 4, 7, ABJURATION);
  spello(SPELL_SLOW, "slow", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT, TRUE, MAG_AFFECTS,
	"You feel the slow spell wear off.", 4, 7,
	ABJURATION);
  spello(SPELL_CIRCLE_A_EVIL, "circle against evil", 58, 43, 1,
     POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS,
	NULL, 7, 7, ABJURATION);
  spello(SPELL_CIRCLE_A_GOOD, "circle against good", 58, 43, 1,
     POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS,
	NULL, 7, 7, ABJURATION);
			/* transmutation */
  spello(SPELL_CUNNING, "cunning", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your magical cunning has faded away.", 3, 7,
	TRANSMUTATION);  // mage 2, cleric 2
  spello(SPELL_WISDOM, "wisdom", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your magical wisdom has faded away.", 3, 7,
	TRANSMUTATION);  // mage 2, cleric 2
  spello(SPELL_CHARISMA, "charisma", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your magical charisma has faded away.", 3, 7,
	TRANSMUTATION);  // mage 2, cleric 2

  
  // 4th circle
			/* evocation */
  spello(SPELL_ICE_STORM, "ice storm", 58, 43, 1, POS_FIGHTING,
	TAR_IGNORE, TRUE, MAG_AREAS,
	NULL, 5, 8, EVOCATION);
  spello(SPELL_FIRE_SHIELD, "fire shield", 37, 22, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"You watch your fire shield fade away.", 5, 8, EVOCATION);
  spello(SPELL_COLD_SHIELD, "cold shield", 37, 22, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"You watch your cold shield fade away.", 5, 8, EVOCATION);
               /* conjuration */
  spello(SPELL_BILLOWING_CLOUD, "billowing cloud", 65, 50, 1, POS_FIGHTING,
	TAR_IGNORE, FALSE, MAG_ROOM,
	"You watch as the thick billowing cloud dissipates.", 7, 8,
	CONJURATION);  
  spello(SPELL_SUMMON_CREATURE_4, "summon creature iv", 95, 80, 1,
     POS_FIGHTING, TAR_IGNORE, FALSE, MAG_SUMMONS, NULL, 8, 8, CONJURATION);
			/* necromancy */
  spello(SPELL_ANIMATE_DEAD, "animate dead", 72, 57, 1, POS_FIGHTING,
	TAR_OBJ_ROOM, FALSE, MAG_SUMMONS,
	NULL, 10, 8, NECROMANCY);
  spello(SPELL_CURSE, "curse", 80, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_OBJ_INV, TRUE, MAG_AFFECTS | MAG_ALTER_OBJS,
	"You feel more optimistic.", 7, 8, NECROMANCY);
			/* enchantment */
			/* illusion */
			/* divination */
  spello(SPELL_WIZARD_EYE, "wizard eye", 65, 50, 1, POS_FIGHTING,
	TAR_IGNORE, FALSE, MAG_MANUAL,
	NULL, 6, 8, DIVINATION);
			/* abjuration */
  spello(SPELL_STONESKIN, "stone skin", 51, 36, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your skin returns to its normal texture.", 3, 8, ABJURATION);
			/* transmutation */

  
  // 5th circle
			/* evocation */
               /* conjuration */
			/* necromancy */
			/* enchantment */
			/* illusion */
			/* divination */
  spello(SPELL_LOCATE_OBJECT, "locate object", 58, 43, 1, POS_FIGHTING,
	TAR_OBJ_WORLD, FALSE, MAG_MANUAL,
	NULL, 9, 9, DIVINATION);
			/* abjuration */
			/* transmutation */
  

  // 6th circle
			/* evocation */
  spello(SPELL_BALL_OF_LIGHTNING, "ball of lightning", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 5, 10, EVOCATION);
               /* conjuration */
			/* necromancy */
			/* enchantment */
			/* illusion */
			/* divination */
			/* abjuration */
			/* transmutation */
  spello(SPELL_CLONE, "clone", 65, 50, 1, POS_FIGHTING,
	TAR_IGNORE, FALSE, MAG_SUMMONS,
	NULL, 9, 10, TRANSMUTATION);

  
  // 7th circle
			/* evocation */
  spello(SPELL_MISSILE_STORM, "missile storm", 72, 57, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 6, 11, EVOCATION);
               /* conjuration */
			/* necromancy */
			/* enchantment */
			/* illusion */
			/* divination */
			/* abjuration */
			/* transmutation */  
  spello(SPELL_TELEPORT, "teleport", 72, 57, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_MANUAL,
	NULL, 1, 11, TRANSMUTATION);

  
  // 8th circle
			/* evocation */
  spello(SPELL_CHAIN_LIGHTNING, "chain lightning", 79, 64, 1, POS_FIGHTING,
	TAR_IGNORE, TRUE, MAG_AREAS,
	NULL, 8, 12, EVOCATION);
               /* conjuration */
			/* necromancy */
			/* enchantment */
			/* illusion */
			/* divination */
			/* abjuration */
			/* transmutation */  
  spello(SPELL_WATERWALK, "waterwalk", 79, 64, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your feet seem less buoyant.", 7, 12, TRANSMUTATION);

  
  // 9th circle
			/* evocation */
  spello(SPELL_METEOR_SWARM, "meteor swarm", 85, 70, 1, POS_FIGHTING,
	TAR_IGNORE, TRUE, MAG_AREAS,
	NULL, 9, 13, EVOCATION);
               /* conjuration */
			/* necromancy */
			/* enchantment */
			/* illusion */
			/* divination */
			/* abjuration */
			/* transmutation */  
  spello(SPELL_POLYMORPH, "polymorph self", 58, 43, 1, POS_FIGHTING,
	TAR_IGNORE, FALSE, MAG_MANUAL,
	NULL, 9, 9, TRANSMUTATION);

  
  // epic magical
  spello(SPELL_EPIC_MAGE_ARMOR, "epic mage armor", 95, 80, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel less protected.", 4, 14, ABJURATION);
  spello(SPELL_EPIC_WARDING, "epic warding", 95, 80, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your massive magical ward dissipates.", 4, 14, ABJURATION);
  // end magical

  // divine spells
  // 1st circle
  spello(SPELL_CURE_LIGHT, "cure light", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_POINTS,
	NULL, 1, 6, NOSCHOOL);
  spello(SPELL_CAUSE_LIGHT_WOUNDS, "cause light wounds", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 2, 6, NOSCHOOL);
  spello(SPELL_ARMOR, "armor", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel less protected.", 4, 6,
	CONJURATION);

  // 2nd circle
  spello(SPELL_CREATE_FOOD, "create food", 37, 22, 1, POS_FIGHTING,
	TAR_IGNORE, FALSE, MAG_CREATIONS,
	NULL, 2, 7, NOSCHOOL);
  spello(SPELL_CREATE_WATER, "create water", 37, 22, 1, POS_FIGHTING,
	TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL,
	NULL, 2, 7, NOSCHOOL);
  spello(SPELL_CAUSE_MODERATE_WOUNDS, "cause moderate wounds", 37, 22, 1, 
	POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 3, 7, NOSCHOOL);

  // 3rd circle
  spello(SPELL_DETECT_ALIGN, "detect alignment", 44, 29, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel less aware.", 3, 8, NOSCHOOL);
  spello(SPELL_CURE_BLIND, "cure blind", 44, 29, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS,
	NULL, 3, 8, NOSCHOOL);
  spello(SPELL_BLESS, "bless", 44, 29, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE, MAG_AFFECTS | MAG_ALTER_OBJS,
	"You feel less righteous.", 3, 8, NOSCHOOL);
  spello(SPELL_CAUSE_SERIOUS_WOUNDS, "cause serious wounds", 44, 29, 1,
	POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 4, 9, NOSCHOOL);

  // 4th circle
  spello(SPELL_CAUSE_CRITICAL_WOUNDS, "cause critical wounds", 51, 36, 1, 
	POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 5, 10, NOSCHOOL);
  spello(SPELL_CURE_CRITIC, "cure critic", 51, 36, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_POINTS,
	NULL, 3, 10, NOSCHOOL);

  // 5th circle
  spello(SPELL_PROT_FROM_EVIL, "protection from evil", 58, 43, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel less protected from evil.", 5, 11, NOSCHOOL);
  spello(SPELL_PROT_FROM_GOOD, "protection from good", 58, 43, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel less protected from good.", 5, 11, NOSCHOOL);
  spello(SPELL_GROUP_ARMOR, "group armor", 58, 43, 1, POS_FIGHTING,
	TAR_IGNORE, FALSE, MAG_GROUPS,
	NULL, 5, 11, NOSCHOOL);
  spello(SPELL_FLAME_STRIKE, "flame strike", 58, 43, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 6, 11, NOSCHOOL);

  // 6th circle
  spello(SPELL_DISPEL_EVIL, "dispel evil", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 5, 12, NOSCHOOL);
  spello(SPELL_DISPEL_GOOD, "dispel good", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 5, 12, NOSCHOOL);
  spello(SPELL_REMOVE_POISON, "remove poison", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_UNAFFECTS | MAG_ALTER_OBJS,
	NULL, 7, 12, NOSCHOOL);
  spello(SPELL_HARM, "harm", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 7, 12, NOSCHOOL);
  spello(SPELL_HEAL, "heal", 65, 50, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_POINTS | MAG_UNAFFECTS,
	NULL, 5, 12, NOSCHOOL);

  // 7th circle
  spello(SPELL_CALL_LIGHTNING, "call lightning", 72, 57, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 8, 13, NOSCHOOL);
  spello(SPELL_CONTROL_WEATHER, "control weather", 72, 57, 1, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_MANUAL,
	NULL, 8, 13, NOSCHOOL);
  spello(SPELL_SUMMON, "summon", 72, 57, 1, POS_FIGHTING,
	TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL,
	NULL, 10, 13, NOSCHOOL);
  spello(SPELL_WORD_OF_RECALL, "word of recall", 72, 57, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_MANUAL,
	NULL, 0, 13, NOSCHOOL);

  // 8th circle
  spello(SPELL_SENSE_LIFE, "sense life", 79, 64, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel less aware of your surroundings.", 8, 14, NOSCHOOL);
  spello(SPELL_SANCTUARY, "sanctuary", 79, 64, 1, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"The white aura around your body fades.", 8, 14, NOSCHOOL);
  spello(SPELL_DESTRUCTION, "destruction", 79, 64, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL, 9, 14, NOSCHOOL);

  // 9th circle
  spello(SPELL_EARTHQUAKE, "earthquake", 85, 70, 1, POS_FIGHTING,
	TAR_IGNORE, TRUE, MAG_AREAS,
	NULL, 10, 15, NOSCHOOL);
  spello(SPELL_GROUP_HEAL, "group heal", 85, 70, 1, POS_FIGHTING,
	TAR_IGNORE, FALSE, MAG_GROUPS,
	NULL, 5, 15, NOSCHOOL);
  // epic divine
  // end divine

  

  /* NON-castable spells should appear below here. */
  spello(SPELL_IDENTIFY, "identify", 0, 0, 0, 0,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL,
	NULL, 0, 0, NOSCHOOL);

  /* you might want to name this one something more fitting to your theme -Welcor*/
  spello(SPELL_DG_AFFECT, "Afflicted", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0,
	NULL, 0, 0, NOSCHOOL);

  /* Declaration of skills - this actually doesn't do anything except set it up
   * so that immortals can use these skills by default.  The min level to use
   * the skill for other classes is set up in class.c. */
  skillo(SKILL_BACKSTAB, "backstab");				//401
  skillo(SKILL_BASH, "bash");
  skillo(SKILL_MUMMY_DUST, "es mummy dust");
  skillo(SKILL_KICK, "kick");
  skillo(SKILL_WEAPON_SPECIALIST, "weapon specialist");	//405
  skillo(SKILL_WHIRLWIND, "whirlwind");
  skillo(SKILL_RESCUE, "rescue");
  skillo(SKILL_DRAGON_KNIGHT, "es dragon knight");
  skillo(SKILL_LUCK_OF_HEROES, "luck of heroes");
  skillo(SKILL_TRACK, "track");					//410
  skillo(SKILL_QUICK_CHANT, "quick chant");
  skillo(SKILL_AMBIDEXTERITY, "ambidexterity");
  skillo(SKILL_DIRTY_FIGHTING, "dirty fighting");
  skillo(SKILL_DODGE, "dodge");
  skillo(SKILL_IMPROVED_CRITICAL, "improved critical");		//415
  skillo(SKILL_MOBILITY, "mobility");
  skillo(SKILL_SPRING_ATTACK, "spring attack");
  skillo(SKILL_TOUGHNESS, "toughness");
  skillo(SKILL_TWO_WEAPON_FIGHT, "two weapon fighting");
  skillo(SKILL_FINESSE, "finesse");				//420
  skillo(SKILL_ARMOR_SKIN, "armor skin");
  skillo(SKILL_BLINDING_SPEED, "blinding speed");
  skillo(SKILL_DAMAGE_REDUC_1, "damage reduction");
  skillo(SKILL_DAMAGE_REDUC_2, "greater damage reduction");
  skillo(SKILL_DAMAGE_REDUC_3, "epic damage reduction");	//425
  skillo(SKILL_EPIC_TOUGHNESS, "epic toughness");
  skillo(SKILL_OVERWHELMING_CRIT, "overwhelming critical");
  skillo(SKILL_SELF_CONCEAL_1, "self concealment");
  skillo(SKILL_SELF_CONCEAL_2, "greater concealment");
  skillo(SKILL_SELF_CONCEAL_3, "epic concealment");		//430
  skillo(SKILL_TRIP, "trip");
  skillo(SKILL_IMPROVED_WHIRL, "improved whirlwind");
  skillo(SKILL_CLEAVE, "cleave (incomplete)");
  skillo(SKILL_GREAT_CLEAVE, "great_cleave (incomplete)");
  skillo(SKILL_SPELLPENETRATE, "spell penetration");		//435
  skillo(SKILL_SPELLPENETRATE_2, "greater spell penetrate");
  skillo(SKILL_PROWESS, "prowess");
  skillo(SKILL_EPIC_PROWESS, "epic prowess");
  skillo(SKILL_EPIC_2_WEAPON, "epic two weapon fighting");
  skillo(SKILL_SPELLPENETRATE_3, "epic spell penetrate");		//440
  skillo(SKILL_SPELL_RESIST_1, "spell resistance");
  skillo(SKILL_SPELL_RESIST_2, "improved spell resist");
  skillo(SKILL_SPELL_RESIST_3, "greater spell resist");
  skillo(SKILL_SPELL_RESIST_4, "epic spell resist");
  skillo(SKILL_SPELL_RESIST_5, "supreme spell resist");		//445
  skillo(SKILL_INITIATIVE, "initiative");
  skillo(SKILL_EPIC_CRIT, "epic critical");
  skillo(SKILL_IMPROVED_BASH, "improved bash");
  skillo(SKILL_IMPROVED_TRIP, "improved trip");
  skillo(SKILL_POWER_ATTACK, "power attack");			//450
  skillo(SKILL_EXPERTISE, "combat expertise");
  skillo(SKILL_GREATER_RUIN, "es greater ruin");
  skillo(SKILL_HELLBALL, "es hellball");
  skillo(SKILL_EPIC_MAGE_ARMOR, "es epic mage armor");
  skillo(SKILL_EPIC_WARDING, "es epic warding");			//455
  skillo(SKILL_RAGE, "rage");			//185
  skillo(SKILL_PROF_MINIMAL, "minimal weapon prof");             //457
  skillo(SKILL_PROF_BASIC, "basic weapon prof");                //458
  skillo(SKILL_PROF_ADVANCED, "advanced weapon prof");              //459
  skillo(SKILL_PROF_MASTER, "master weapon prof");            //460
  skillo(SKILL_PROF_EXOTIC, "exotic weapon prof");             //461
  skillo(SKILL_PROF_LIGHT_A, "light armor prof");              //462
  skillo(SKILL_PROF_MEDIUM_A, "medium armor prof");             //463
  skillo(SKILL_PROF_HEAVY_A, "heavy armor prof");              //464
  skillo(SKILL_PROF_SHIELDS, "shield prof");              //465
  skillo(SKILL_PROF_T_SHIELDS, "tower shield prof");            //466
  skillo(SKILL_MURMUR, "murmur (incomplete)");
  skillo(SKILL_PROPAGANDA, "propaganda (incomplete)");
  skillo(SKILL_LOBBY, "lobby (incomplete)");                  //469
  skillo(SKILL_STUNNING_FIST, "stunning fist");             //470
  skillo(SKILL_MINING, "mining (incomplete)");             //471
  skillo(SKILL_HUNTING, "hunting (incomplete)");             //472
  skillo(SKILL_FORESTING, "foresting (incomplete)");             //473
  skillo(SKILL_KNITTING, "knitting (incomplete)");             //474
  skillo(SKILL_CHEMISTRY, "chemistry (incomplete)");             //475
  skillo(SKILL_ARMOR_SMITHING, "armor smithing (incomplete)");             //476
  skillo(SKILL_WEAPON_SMITHING, "weapon smithing (incomplete)");             //477
  skillo(SKILL_JEWELRY_MAKING, "jewelry making (incomplete)");             //478
  skillo(SKILL_LEATHER_WORKING, "leather working (incomplete)");             //479
  skillo(SKILL_FAST_CRAFTER, "fast crafter (incomplete)");             //480
  skillo(SKILL_BONE_ARMOR, "bone armor (incomplete)");             //481
  skillo(SKILL_ELVEN_CRAFTING, "elvent crafting (incomplete)");             //482
  skillo(SKILL_MASTERWORK_CRAFTING, "masterwork crafting (incomplete)");             //483
  skillo(SKILL_DRACONIC_CRAFTING, "draconic crafting (incomplete)");             //484
  skillo(SKILL_DWARVEN_CRAFTING, "dwarven crafting (incomplete)");             //485
  skillo(SKILL_LIGHTNING_REFLEXES, "lightning reflexes");             //486
  skillo(SKILL_GREAT_FORTITUDE, "great fortitude");             //487
  skillo(SKILL_IRON_WILL, "iron will");             //488
  skillo(SKILL_EPIC_REFLEXES, "epic reflexes");             //489
  skillo(SKILL_EPIC_FORTITUDE, "epic fortitude");             //490
  skillo(SKILL_EPIC_WILL, "epic will");             //491
  skillo(SKILL_SHIELD_SPECIALIST, "shield specialist");             //492
  /****note weapon specialist and luck of heroes inserted in free slots ***/

}
