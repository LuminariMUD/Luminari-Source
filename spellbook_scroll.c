/****************************************************************************
 *  Realms of Luminari
 *  File:     spellbook_scroll.c
 *  Usage:    scroll related functions
 *            spellbook-related functions are here too
 *  Header:   Header Info is in spells.h
 *  Authors:  Spellbook functions taken from CWG project, adapted by Zusuk
 ****************************************************************************/

/*** TODO:  03/14/2013 reported - move header info into separate file or
 neatly organize in spells.h */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "spells.h"
#include "db.h"
#include "comm.h"
#include "mud_event.h"
#include "constants.h"
#include "act.h"
#include "handler.h"    // for obj_from_char()
#include "spec_procs.h" // for compute_ability
#include "spell_prep.h"

/* local, global variables, defines */
char buf[MAX_INPUT_LENGTH] = {'\0'};
bool isSorcBloodlineSpell(int bloodline, int spellnum);
int getSorcBloodline(struct char_data *ch);
#define TERMINATE 0

/* =============================================== */
/* ==================Spellbooks=================== */
/* =============================================== */

/* this function displays the contents of a scroll */
void display_scroll(struct char_data *ch, struct obj_data *obj)
{
  if (GET_OBJ_TYPE(obj) != ITEM_SCROLL)
    return;

  send_to_char(ch, "The scroll contains the following spell(s):\r\n");
  send_to_char(ch, "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\r\n");
  if (GET_OBJ_VAL(obj, 1) >= 1)
    send_to_char(ch, "%-20s\r\n", skill_name(GET_OBJ_VAL(obj, 1)));
  if (GET_OBJ_VAL(obj, 2) >= 1)
    send_to_char(ch, "%-20s\r\n", skill_name(GET_OBJ_VAL(obj, 2)));
  if (GET_OBJ_VAL(obj, 3) >= 1)
    send_to_char(ch, "%-20s\r\n", skill_name(GET_OBJ_VAL(obj, 3)));

  return;
}

/* This function displays the contents of ch's given spellbook */
void display_spells(struct char_data *ch, struct obj_data *obj)
{
  int i;

  if (GET_OBJ_TYPE(obj) != ITEM_SPELLBOOK)
    return;

  if (!obj->sbinfo)
  {
    send_to_char(ch, "This spellbook is completely unused...\r\n");
    return;
  }

  send_to_char(ch, "The spellbook contains the following spell(s):\r\n");
  send_to_char(ch, "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\r\n");

  for (i = 0; i < SPELLBOOK_SIZE; i++)
  {
    if (obj->sbinfo[i].spellname)
      send_to_char(ch, "%-20s		[%2d]\r\n",
                   spell_info[obj->sbinfo[i].spellname].name,
                   ((spell_info[obj->sbinfo[i].spellname].min_level[CLASS_WIZARD] + 1) / 2));
  }

  return;
}

/* this function checks whether the spellbook a ch has
 * given spellnum
 * Input:  obj (spellbook), spellnum of spell we're looking for
 * Output:  returns TRUE if spell found in book, otherwise FALSE
 */
bool spell_in_book(struct obj_data *obj, int spellnum)
{
  int i = 0;

  if (!obj)
    return FALSE;

  if (GET_OBJ_TYPE(obj) != ITEM_SPELLBOOK)
    return FALSE;

  if (!obj->sbinfo)
    return FALSE;

  for (i = 0; i < SPELLBOOK_SIZE; i++)
    if (obj->sbinfo[i].spellname == spellnum)
      return TRUE;

  return FALSE;
}

/* this function checks whether the scroll a ch has
 * given spellnum
 * Input:  obj (scroll), spellnum of spell we're looking for
 * Output:  returns TRUE if spell found in the scroll, otherwise FALSE
 */
int spell_in_scroll(struct obj_data *obj, int spellnum)
{
  if (!obj)
    return FALSE;

  if (GET_OBJ_TYPE(obj) != ITEM_SCROLL)
    return FALSE;

  if (GET_OBJ_VAL(obj, 1) == spellnum ||
      GET_OBJ_VAL(obj, 2) == spellnum ||
      GET_OBJ_VAL(obj, 3) == spellnum)
    return TRUE;

  return FALSE;
}

/* this function is the function to check to see if the ch:
 * 1)  has a spellbook
 * 2)  then calls another function to see if the spell is in the book
 * 3)  it will also check if the spell happens to be in a scroll
 * Input:  ch, spellnum, class of ch related to gen_mem command
 * Output:  returns TRUE if found, FALSE if not found
 */
bool spellbook_ok(struct char_data *ch, int spellnum, int class, bool check_scroll)
{
  struct obj_data *obj = NULL;
  bool found = FALSE;
  int i = 0;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return TRUE;

  if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch))
  {
    send_to_char(ch, "It is too dark to study!\r\n");
    return FALSE;
  }
  if (AFF_FLAGGED(ch, AFF_BLIND) &&
      !HAS_FEAT(ch, FEAT_BLINDSENSE))
  {
    send_to_char(ch, "You are blind!\r\n");
    return FALSE;
  }

  if (class == CLASS_WIZARD)
  {

    /* for-loop for inventory */
    for (obj = ch->carrying; obj && !found; obj = obj->next_content)
    {
      if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK)
      {
        if (spell_in_book(obj, spellnum))
        {
          found = TRUE;
          break;
        }
        continue;
      }

      if (GET_OBJ_TYPE(obj) == ITEM_SCROLL && check_scroll)
      {
        if (spell_in_scroll(obj, spellnum) && CLASS_LEVEL(ch, class) >=
                                                  spell_info[spellnum].min_level[class])
        {
          found = TRUE;
          send_to_char(ch, "The \tmmagical energy\tn of the scroll leaves the "
                           "paper and enters your \trmind\tn!\r\n");
          send_to_char(ch, "With the \tmmagical energy\tn transfered from the "
                           "scroll, the scroll withers to dust!\r\n");
          obj_from_char(obj);
          break;
        }
        continue;
      }
    }

    /* for-loop for gear */
    for (i = 0; i < NUM_WEARS; i++)
    {
      if (GET_EQ(ch, i))
        obj = GET_EQ(ch, i);
      else
        continue;

      if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK)
      {
        if (spell_in_book(obj, spellnum))
        {
          found = TRUE;
          break;
        }
        continue;
      }

      if (GET_OBJ_TYPE(obj) == ITEM_SCROLL && check_scroll)
      {
        if (spell_in_scroll(obj, spellnum) && CLASS_LEVEL(ch, class) >=
                                                  spell_info[spellnum].min_level[class])
        {
          found = TRUE;
          send_to_char(ch, "The \tmmagical energy\tn of the scroll leaves the "
                           "paper and enters your \trmind\tn!\r\n");
          send_to_char(ch, "With the \tmmagical energy\tn transfered from the "
                           "scroll, the scroll withers to dust!\r\n");
          obj_from_char(obj);
          break;
        }
        continue;
      }
    }

    if (!found)
    {
      if (check_scroll)
        send_to_char(ch, "You don't seem to have %s in your spellbook or in "
                         "any scrolls.\r\n",
                     spell_info[spellnum].name);
      return FALSE;
    }
  }
  else //classes besides wizard
    return FALSE;

  return TRUE;
}

/* used for scribing spells from scrolls to spellbook
   used for scribing spells from memory to scrolls
 TODO:  scribing spells from memory to spellbook
 */
ACMD(do_scribe)
{
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  char *s = NULL, buf[READ_SIZE] = {'\0'};
  int i = 0, spellnum = -1, found = FALSE;
  struct obj_data *obj = NULL, *scroll = NULL, *next_obj = NULL;

  half_chop(argument, arg1, arg2);

  /* quick outs */
  if (!*arg1 || !*arg2)
  {
    send_to_char(ch, "Usually you scribe SOMETHING.\r\n");
    return;
  }
  if (!HAS_FEAT(ch, FEAT_SCRIBE_SCROLL))
  {
    send_to_char(ch, "You really aren't qualified to do that...\r\n");
    return;
  }
  if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)))
  {
    send_to_char(ch, "You don't seem to have any %ss.\r\n", arg1);
    return;
  }

  s = strtok(arg2, "\0");
  spellnum = find_skill_num(s);

  if ((spellnum < 1) || (spellnum >= NUM_SPELLS))
  {
    send_to_char(ch, "Strange, there is no such spell.\r\n");
    return;
  }

  /* only wizard spells */
  if ((LVL_IMMORT - 1) < spell_info[spellnum].min_level[CLASS_WIZARD])
  {
    send_to_char(ch, "That spell is not appropriate for scribing!\r\n");
    return;
  }

  /* very much just a dummy check*/
  if (!GET_SKILL(ch, spellnum))
  {
    send_to_char(ch, "That spell is not fitting!\r\n");
    return;
  }

  /* found an object, looking for a spellbook or scroll.. */
  if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK)
  {
    /* check if spell is already in book */
    if (spell_in_book(obj, spellnum))
    {
      send_to_char(ch, "You already have the spell '%s' in this spellbook.\r\n",
                   spell_info[spellnum].name);
      return;
    }
    /* initiate spellbook if needed */
    if (!obj->sbinfo)
    {
      CREATE(obj->sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
      memset((char *)obj->sbinfo, 0,
             SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
    }
    /* look for empty spot in book */
    for (i = 0; i < SPELLBOOK_SIZE; i++)
      if (obj->sbinfo[i].spellname == 0)
        break;

    /* oops no space */
    if (i == SPELLBOOK_SIZE)
    {
      send_to_char(ch, "Your spellbook is full!\r\n");
      return;
    }

    /* ok now loop through inventory, check if we have a scroll
       with requested spell name in it */
    for (scroll = ch->carrying; scroll; scroll = next_obj)
    {
      next_obj = scroll->next_content;

      if (spell_in_scroll(scroll, spellnum))
      {
        send_to_char(ch, "You use a scroll with the spell in your "
                         "inventory...\r\n");
        found = TRUE;
        break;
      }
    }

    if (!found || !scroll)
    {
      send_to_char(ch, "You must have a scroll with that spell in order"
                       " to scribe it!\r\n");
      return;
    }

    /*
    if (!hasSpell(ch, spellnum)) {
      send_to_char(ch, "You must have the spell committed to memory before "
              "you can scribe it!\r\n");
      return;
    }
     */

    obj->sbinfo[i].spellname = spellnum;
    obj->sbinfo[i].pages = MAX(1, lowest_spell_level(spellnum) / 2);
    send_to_char(ch, "You scribe the spell '%s' into your spellbook, which "
                     "takes up %d pages.\r\n",
                 spell_info[spellnum].name,
                 obj->sbinfo[i].pages);
    extract_obj(scroll);
    save_char(ch, 0);
  }
  else if (GET_OBJ_TYPE(obj) == ITEM_SCROLL)
  {

    if (GET_OBJ_VAL(obj, 1) > 0 ||
        GET_OBJ_VAL(obj, 2) > 0 ||
        GET_OBJ_VAL(obj, 3) > 0)
    {
      send_to_char(ch, "The scroll has a spell inscribed on it!\r\n");
      return;
    }

    if (!is_spell_in_collection(ch, CLASS_WIZARD, spellnum, METAMAGIC_NONE))
    {
      send_to_char(ch, "You must have the spell committed to memory before "
                       "you can scribe it!\r\n");
      return;
    }

    GET_OBJ_VAL(obj, 0) = GET_LEVEL(ch);
    GET_OBJ_VAL(obj, 1) = spellnum;
    GET_OBJ_VAL(obj, 2) = -1;
    GET_OBJ_VAL(obj, 3) = -1;

    found = FALSE;

    sprintf(buf, "a scroll of '%s'", spell_info[spellnum].name);
    obj->short_description = strdup(buf);
    send_to_char(ch, "You scribe the spell '%s' onto %s.\r\n",
                 spell_info[spellnum].name, obj->short_description);
  }
  else
  {
    send_to_char(ch, "But you don't have anything suitable for scribing!\r\n");
    return;
  }

  if (!found)
  {
    send_to_char(ch, "The magical energy committed for the spell '%s' has been "
                     "expended.\r\n",
                 spell_info[spellnum].name);
    sprintf(buf, "%d", spellnum);
    collection_remove_by_class(ch, CLASS_WIZARD, spellnum, METAMAGIC_NONE);
  }
}

/***  end command functions ***/
#undef TERMINATE
