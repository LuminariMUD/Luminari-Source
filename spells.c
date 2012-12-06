/**************************************************************************
*  File: spells.c                                          Part of tbaMUD *
*  Usage: Implementation of "manual spells."                              *
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
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "act.h"
#include "fight.h"
#include "mud_event.h"

/* Special spells appear below. */

  /* The "return" of the event function is the time until the event is called
   * again. If we return 0, then the event is freed and removed from the list, but
   * any other numerical response will be the delay until the next call */
EVENTFUNC(event_acid_arrow)
{
  struct char_data *ch, *victim = NULL;
  struct mud_event_data *pMudEvent;
  	
  /* This is just a dummy check, but we'll do it anyway */
  if (event_obj == NULL)
    return 0;
	  
  /* For the sake of simplicity, we will place the event data in easily
   * referenced pointers */  
  pMudEvent = (struct mud_event_data *) event_obj;
  ch = (struct char_data *) pMudEvent->pStruct;    
  if (ch && FIGHTING(ch))  //assign victim, if none escape
    victim = FIGHTING(ch);
  else
    return 0;

  if (ch == NULL || victim == NULL)
    return 0;
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return 0;
  }

  damage(ch, victim, dice(3, 6), SPELL_ACID_ARROW, DAM_ACID,
                FALSE);
  
  update_pos(ch);  
  return 0;
}

  
ASPELL(spell_acid_arrow)
{
  int x = 0;
  
  if (ch == NULL || victim == NULL)
    return;
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  send_to_char(ch, "You send out an arrow of acid whizzing towards your opponent!\r\n");
  act("$n sends out an arrow of acid whizzing!", FALSE, ch, 0, 0, TO_ROOM);
  
  for (x = 0; x < (GET_LEVEL(ch)/3); x++) {
    NEW_EVENT(eACIDARROW, ch, NULL, ((x*6) * PASSES_PER_SEC));  
  }
}


ASPELL(spell_create_water)
{
  int water;

  if (ch == NULL || obj == NULL)
    return;
  /* level = MAX(MIN(level, LVL_IMPL), 1);	 - not used */

  if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
    if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0)) {
      name_from_drinkcon(obj);
      GET_OBJ_VAL(obj, 2) = LIQ_SLIME;
      name_to_drinkcon(obj, LIQ_SLIME);
    } else {
      water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
      if (water > 0) {
	if (GET_OBJ_VAL(obj, 1) >= 0)
	  name_from_drinkcon(obj);
	GET_OBJ_VAL(obj, 2) = LIQ_WATER;
	GET_OBJ_VAL(obj, 1) += water;
	name_to_drinkcon(obj, LIQ_WATER);
	weight_change_object(obj, water);
	act("$p is filled.", FALSE, ch, obj, 0, TO_CHAR);
      }
    }
  }
}

ASPELL(spell_recall)
{
  if (victim == NULL || IS_NPC(victim))
    return;

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(victim)), ZONE_NOASTRAL)) {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, r_mortal_start_room);
  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}

ASPELL(spell_teleport)
{
  room_rnum to_room;

  if (victim == NULL || IS_NPC(victim))
    return;

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(victim)), ZONE_NOASTRAL)) {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  do {
    to_room = rand_number(0, top_of_world);
  } while (ROOM_FLAGGED(to_room, ROOM_PRIVATE) || ROOM_FLAGGED(to_room, ROOM_DEATH) ||
           ROOM_FLAGGED(to_room, ROOM_GODROOM) || ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_CLOSED) ||
           ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_NOASTRAL));

  act("$n slowly fades out of existence and is gone.",
      FALSE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, to_room);
  act("$n slowly fades into existence.", FALSE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}


#define SUMMON_FAIL "You failed.\r\n"
ASPELL(spell_summon)
{
  if (ch == NULL || victim == NULL)
    return;

  if (GET_LEVEL(victim) > MIN(LVL_IMMORT - 1, level + 3)) {
    send_to_char(ch, "%s", SUMMON_FAIL);
    return;
  }

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(victim)), ZONE_NOASTRAL) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_NOASTRAL)) {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  if (!CONFIG_PK_ALLOWED) {
    if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
      act("As the words escape your lips and $N travels\r\n"
	  "through time and space towards you, you realize that $E is\r\n"
	  "aggressive and might harm you, so you wisely send $M back.",
	  FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
	!PLR_FLAGGED(victim, PLR_KILLER)) {
      send_to_char(victim, "%s just tried to summon you to: %s.\r\n"
	      "%s failed because you have summon protection on.\r\n"
	      "Type NOSUMMON to allow other players to summon you.\r\n",
	      GET_NAME(ch), world[IN_ROOM(ch)].name,
	      (ch->player.sex == SEX_MALE) ? "He" : "She");

      send_to_char(ch, "You failed because %s has summon protection on.\r\n", GET_NAME(victim));
      mudlog(BRF, LVL_IMMORT, TRUE, "%s failed summoning %s to %s.", GET_NAME(ch), GET_NAME(victim), world[IN_ROOM(ch)].name);
      return;
    }
  }

  if (mag_resistance(ch, victim, 0))
    return;
  if (MOB_FLAGGED(victim, MOB_NOSUMMON)) {
    send_to_char(ch, "Your victim seems unsummonable.");
    return;
  }
  if (IS_NPC(victim) && mag_savingthrow(ch, victim, SAVING_WILL, 0)) {
    send_to_char(ch, "%s", SUMMON_FAIL);
    return;
  }

  act("$n disappears suddenly.", TRUE, victim, 0, 0, TO_ROOM);

  char_from_room(victim);
  char_to_room(victim, IN_ROOM(ch));

  act("$n arrives suddenly.", TRUE, victim, 0, 0, TO_ROOM);
  act("$n has summoned you!", FALSE, ch, 0, victim, TO_VICT);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}

/* Used by the locate object spell to check the alias list on objects */
int isname_obj(char *search, char *list)
{
  char *found_in_list; /* But could be something like 'ring' in 'shimmering.' */
  char searchname[128];
  char namelist[MAX_STRING_LENGTH];
  int found_pos = -1;
  int found_name=0; /* found the name we're looking for */
  int match = 1;
  int i;

  /* Force to lowercase for string comparisons */
  sprintf(searchname, "%s", search);
  for (i = 0; searchname[i]; i++)
    searchname[i] = LOWER(searchname[i]);

  sprintf(namelist, "%s", list);
  for (i = 0; namelist[i]; i++)
    namelist[i] = LOWER(namelist[i]);

  /* see if searchname exists any place within namelist */
  found_in_list = strstr(namelist, searchname);
  if (!found_in_list) {
    return 0;
  }

  /* Found the name in the list, now see if it's a valid hit. The following
   * avoids substrings (like ring in shimmering) is it at beginning of
   * namelist? */
  for (i = 0; searchname[i]; i++)
    if (searchname[i] != namelist[i])
      match = 0;

  if (match) /* It was found at the start of the namelist string. */
    found_name = 1;
  else { /* It is embedded inside namelist. Is it preceded by a space? */
    found_pos = found_in_list - namelist;
    if (namelist[found_pos-1] == ' ')
      found_name = 1;
  }

  if (found_name)
    return 1;
  else
    return 0;
}


ASPELL(spell_polymorph)
{
  char arg[MAX_INPUT_LENGTH];
  int form = -1;
        
  if (IS_NPC(ch) || !ch->desc)
    return;
        
  one_argument(cast_arg2, arg);
        
  if (!*arg) {
    if (!IS_MORPHED(ch)) {
      send_to_char(ch, "You are already in your natural form!\r\n");
    } else {
      send_to_char(ch, "You shift back into your natural form...\r\n");
      act("$n shifts back to his natural form.", TRUE, ch, 0, 0, TO_ROOM);
      IS_MORPHED(ch) = 0;
    }
    list_forms(ch);
  } else {
    form = atoi(arg);
    if (form < 1 || form > NUM_NPC_RACES - 1) {
      send_to_char(ch, "That is not a valid race!\r\n");
      list_forms(ch);
      return;
    }
    IS_MORPHED(ch) = form;
    send_to_char(ch, "You transform into a %s!\r\n", RACE_ABBR(ch));
    send_to_char(ch, "\tDType 'innates' to see your abilities.\tn\r\n");
    act("$n shapechanges!", TRUE, ch, 0, 0, TO_ROOM);
  }
}


ASPELL(spell_locate_object)
{
  struct obj_data *i;
  char name[MAX_INPUT_LENGTH];
  int j;

  if (!obj) {
    send_to_char(ch, "You sense nothing.\r\n");
    return;
  }

  /*  added a global var to catch 2nd arg. */
  sprintf(name, "%s", cast_arg2);

  j = CASTER_LEVEL(ch) / 2;  /* # items to show = twice char's level */

  for (i = object_list; i && (j > 0); i = i->next) {
    if (!isname_obj(name, i->name))
      continue;

  send_to_char(ch, "%c%s", UPPER(*i->short_description), i->short_description + 1);

    if (i->carried_by)
      send_to_char(ch, " is being carried by %s.\r\n", PERS(i->carried_by, ch));
    else if (IN_ROOM(i) != NOWHERE)
      send_to_char(ch, " is in %s.\r\n", world[IN_ROOM(i)].name);
    else if (i->in_obj)
      send_to_char(ch, " is in %s.\r\n", i->in_obj->short_description);
    else if (i->worn_by)
      send_to_char(ch, " is being worn by %s.\r\n", PERS(i->worn_by, ch));
    else
      send_to_char(ch, "'s location is uncertain.\r\n");

    j--;
  }
}

ASPELL(spell_charm)  // enchantment
{
  struct affected_type af;
  int elf_bonus = 0;
  
  if (victim == NULL || ch == NULL)
    return;

  if (GET_RACE(victim) == RACE_ELF ||  //elven enchantment resistance
          GET_RACE(victim) == RACE_H_ELF)
    elf_bonus += 2;
  
  if (victim == ch)
    send_to_char(ch, "You like yourself even better!\r\n");

  else if (MOB_FLAGGED(victim, MOB_NOCHARM)) {
    send_to_char(ch, "Your victim doesn't seem vulnerable to charm "
            "enchantments!\r\n");
    if (IS_NPC(victim))
      hit(victim, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
  }

  else if (AFF_FLAGGED(ch, AFF_CHARM))
    send_to_char(ch, "You can't have any followers of your own!\r\n");

  else if (AFF_FLAGGED(victim, AFF_CHARM))
    send_to_char(ch, "Your victim is already charmed.\r\n");

  else if (CASTER_LEVEL(ch) <= GET_LEVEL(victim))
    send_to_char(ch, "Your victim is too powerful.\r\n");

  /* player charming another player - no legal reason for this */
  else if (!CONFIG_PK_ALLOWED && !IS_NPC(victim))
    send_to_char(ch, "You fail - shouldn't be doing it anyway.\r\n");

  else if (circle_follow(victim, ch))
    send_to_char(ch, "Sorry, following in circles is not allowed.\r\n");

  else if (mag_resistance(ch, victim, 0)) {
    send_to_char(ch, "You failed to penetrate the spell resistance!");
    if (IS_NPC(victim))
      hit(victim, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
  }

  else if (mag_savingthrow(ch, victim, SAVING_WILL, elf_bonus)) {
    send_to_char(ch, "Your victim resists!\r\n");
    if (IS_NPC(victim))
      hit(victim, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);

  } else {
    if (victim->master)
      stop_follower(victim);

    add_follower(victim, ch);

    new_affect(&af);
    af.spell = SPELL_CHARM;
    af.duration = 100;
    if (GET_CHA_BONUS(ch))
      af.duration *= GET_CHA_BONUS(ch) * 25;
    SET_BIT_AR(af.bitvector, AFF_CHARM);
    affect_to_char(victim, &af);

    act("Isn't $n just such a nice fellow?", FALSE, ch, 0, victim, TO_VICT);
//    if (IS_NPC(victim))
//      REMOVE_BIT_AR(MOB_FLAGS(victim), MOB_SPEC);
  }
  // should never get here
}


ASPELL(spell_identify)  // divination
{
  int i, found;
  size_t len;

  if (obj) {
    char bitbuf[MAX_STRING_LENGTH];

    sprinttype(GET_OBJ_TYPE(obj), item_types, bitbuf, sizeof(bitbuf));
    send_to_char(ch, "You feel informed:\r\nObject '%s', Item type: %s\r\n", obj->short_description, bitbuf);

    sprintbitarray(GET_OBJ_WEAR(obj), wear_bits, TW_ARRAY_MAX, bitbuf);
    send_to_char(ch, "Can be worn on: %s\r\n", bitbuf);

    if (GET_OBJ_AFFECT(obj)) {
      sprintbitarray(GET_OBJ_AFFECT(obj), affected_bits, AF_ARRAY_MAX, bitbuf);
      send_to_char(ch, "Item will give you following abilities:  %s\r\n", bitbuf);
    }

    sprintbitarray(GET_OBJ_EXTRA(obj), extra_bits, EF_ARRAY_MAX, bitbuf);
    send_to_char(ch, "Item is: %s\r\n", bitbuf);

    send_to_char(ch, "Size: %s, Material: %s.\r\n",
            size_names[GET_OBJ_SIZE(obj)],
            material_name[GET_OBJ_MATERIAL(obj)]);
    
    send_to_char(ch, "Weight: %d, Value: %d, Rent: %d, Min. level: %d\r\n",
                     GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj), GET_OBJ_LEVEL(obj));

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_SCROLL:
    case ITEM_POTION:
      len = i = 0;
      int hasVal = 0;

      if (GET_OBJ_VAL(obj, 1) >= 1) {
	i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(GET_OBJ_VAL(obj, 1)));
        if (i >= 0)
          len += i;
        hasVal++;
      }

      if (GET_OBJ_VAL(obj, 2) >= 1 && len < sizeof(bitbuf)) {
	i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(GET_OBJ_VAL(obj, 2)));
        if (i >= 0)
          len += i;
        hasVal++;
      }

      if (GET_OBJ_VAL(obj, 3) >= 1 && len < sizeof(bitbuf)) {
	i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(GET_OBJ_VAL(obj, 3)));
        if (i >= 0)
          len += i;
        hasVal++;
      }

      if (hasVal)
        send_to_char(ch, "This %s casts: %s\r\n", item_types[(int) GET_OBJ_TYPE(obj)],
		bitbuf);
      else
        send_to_char(ch, "This item has no spells imbued in it.\t\n");
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      send_to_char(ch, "This %s casts: %s\r\nIt has %d maximum charge%s and %d remaining.\r\n",
		item_types[(int) GET_OBJ_TYPE(obj)], skill_name(GET_OBJ_VAL(obj, 3)),
		GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s", GET_OBJ_VAL(obj, 2));
      break;
    case ITEM_WEAPON:
      send_to_char(ch, "Damage Dice is '%dD%d' for an average per-round damage of %.1f.\r\n",
		GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), ((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1));
      send_to_char(ch, "Weapon Type: %s\r\n", attack_hit_text[GET_OBJ_VAL(obj, 3)].singular);
      send_to_char(ch, "Proficiency: %s\r\n", item_profs[GET_OBJ_PROF(obj)]);
      break;
    case ITEM_ARMOR:
      send_to_char(ch, "AC-apply is %d\r\n", GET_OBJ_VAL(obj, 0));
      send_to_char(ch, "Proficiency: %s\r\n", item_profs[GET_OBJ_PROF(obj)]);
      break;
    }
    found = FALSE;
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
      if ((obj->affected[i].location != APPLY_NONE) &&
	  (obj->affected[i].modifier != 0)) {
	if (!found) {
	  send_to_char(ch, "Can affect you as :\r\n");
	  found = TRUE;
	}
	sprinttype(obj->affected[i].location, apply_types, bitbuf, sizeof(bitbuf));
	send_to_char(ch, "   Affects: %s By %d\r\n", bitbuf, obj->affected[i].modifier);
      }
    }
  } else if (victim) {		/* victim */
    send_to_char(ch, "Name: %s\r\n", GET_NAME(victim));
    if (!IS_NPC(victim))
      send_to_char(ch, "%s is %d years, %d months, %d days and %d hours old.\r\n",
	      GET_NAME(victim), age(victim)->year, age(victim)->month,
	      age(victim)->day, age(victim)->hours);
    send_to_char(ch, "Alignment: %d.\r\n", GET_ALIGNMENT(victim));
    send_to_char(ch, "Height %d cm, Weight %d pounds\r\n", GET_HEIGHT(victim), GET_WEIGHT(victim));
    send_to_char(ch, "Level: %d, Hits: %d, Mana: %d\r\n", GET_LEVEL(victim), GET_HIT(victim), GET_MANA(victim));
    send_to_char(ch, "AC: %d, Hitroll: %d, Damroll: %d\r\n", compute_armor_class(NULL, victim), GET_HITROLL(victim), GET_DAMROLL(victim));
    send_to_char(ch, "Str: %d/%d, Int: %d, Wis: %d, Dex: %d, Con: %d, Cha: %d\r\n",
	GET_STR(victim), GET_ADD(victim), GET_INT(victim),
	GET_WIS(victim), GET_DEX(victim), GET_CON(victim), GET_CHA(victim));
  }
}


/* Cannot use this spell on an equipped object or it will mess up the wielding
 * character's hit/dam totals. */
ASPELL(spell_enchant_weapon)  // enchantment
{
  int i;

  if (ch == NULL || obj == NULL)
    return;

  /* Either already enchanted or not a weapon. */
  if (GET_OBJ_TYPE(obj) != ITEM_WEAPON || OBJ_FLAGGED(obj, ITEM_MAGIC))
    return;

  /* Make sure no other affections. */
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (obj->affected[i].location != APPLY_NONE)
      return;

  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

  obj->affected[0].location = APPLY_HITROLL;
  obj->affected[0].modifier = MAX(1, (int)(MAGIC_LEVEL(ch) / 10));

  obj->affected[1].location = APPLY_DAMROLL;
  obj->affected[1].modifier = MAX(1, (int)(MAGIC_LEVEL(ch) / 10));

  if (IS_GOOD(ch)) {
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
    act("$p glows \tBblue\tn.", FALSE, ch, obj, 0, TO_CHAR);
  } else if (IS_EVIL(ch)) {
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
    act("$p glows \tRred\tn.", FALSE, ch, obj, 0, TO_CHAR);
  } else
    act("$p glows \tYyellow\tn.", FALSE, ch, obj, 0, TO_CHAR);
}


ASPELL(spell_detect_poison)
{
  if (victim) {
    if (victim == ch) {
      if (AFF_FLAGGED(victim, AFF_POISON))
        send_to_char(ch, "You can sense poison in your blood.\r\n");
      else
        send_to_char(ch, "You feel healthy.\r\n");
    } else {
      if (AFF_FLAGGED(victim, AFF_POISON))
        act("You sense that $E is poisoned.", FALSE, ch, 0, victim, TO_CHAR);
      else
        act("You sense that $E is healthy.", FALSE, ch, 0, victim, TO_CHAR);
    }
  }

  if (obj) {
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
    case ITEM_FOOD:
      if (GET_OBJ_VAL(obj, 3))
	act("You sense that $p has been contaminated.",FALSE,ch,obj,0,TO_CHAR);
      else
	act("You sense that $p is safe for consumption.", FALSE, ch, obj, 0,
	    TO_CHAR);
      break;
    default:
      send_to_char(ch, "You sense that it should not be consumed.\r\n");
    }
  }
}
