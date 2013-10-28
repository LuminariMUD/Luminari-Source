/**************************************************************************
*  File: zone_procs.c                                 Part of LuminariMUD *
*  Usage: Special procedures for zones                                    *
*                                                                         *
*  Header File:  spec_procs.h                                             *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "act.h"
#include "spec_procs.h" /**< zone_procs.c is part of the spec_procs module */
#include "fight.h"
#include "graph.h"
#include "mud_event.h"


/* local, file scope restricted functions */

static mob_vnum castle_virtual(mob_vnum offset);
static room_rnum castle_real_room(room_vnum roomoffset);
static struct char_data *find_npc_by_name(struct char_data *chAtChar, const char *pszName, int iLen);
static int block_way(struct char_data *ch, int cmd, char *arg, room_vnum iIn_room, int iProhibited_direction);
static int member_of_staff(struct char_data *chChar);
static int member_of_royal_guard(struct char_data *chChar);
static struct char_data *find_guard(struct char_data *chAtChar);
static struct char_data *get_victim(struct char_data *chAtChar);
static int banzaii(struct char_data *ch);
static int is_trash(struct obj_data *i);
static void fry_victim(struct char_data *ch);
static int castle_cleaner(struct char_data *ch, int cmd, int gripe);
static int castle_twin_proc(struct char_data *ch, int cmd, char *arg, int ctlnum, const char *twinname);
static void castle_mob_spec(mob_vnum mobnum, SPECIAL(*specproc));

/* end head of file */





/******************************************************************/
/*  KINGS CASTLE */
/******************************************************************/

/* Special procedures for Kings Castle by Pjotr. Coded by Sapowox. */
SPECIAL(CastleGuard);
SPECIAL(James);
SPECIAL(cleaning);
SPECIAL(DicknDavid);
SPECIAL(tim);
SPECIAL(tom);
SPECIAL(king_welmar);
SPECIAL(training_master);
SPECIAL(peter);
SPECIAL(jerry);

/* IMPORTANT! The below defined number is the zone number of the Kings Castle.
 * Change it to apply to your chosen zone number.
 * */
#define Z_KINGS_C 150

/* Assign castle special procedures. NOTE: The mobile number isn't fully 
 * specified. It's only an offset from the zone's base. */
static void castle_mob_spec(mob_vnum mobnum, SPECIAL(*specproc))
{
  mob_vnum vmv = castle_virtual(mobnum);
  mob_rnum rmr = NOBODY;

  if (vmv != NOBODY)
    rmr = real_mobile(vmv);

  if (rmr == NOBODY) {
    if (!mini_mud)
      log("SYSERR: assign_kings_castle(): can't find mob #%d.", vmv);
      /* SYSERR_DESC: When the castle_mob_spec() function is given a mobnum
       * that does not correspond to a mod loaded (when not in minimud mode),
       * this error will result. */
  } else
    mob_index[rmr].func = specproc;
}

static mob_vnum castle_virtual(mob_vnum offset)
{
  zone_rnum zon;

  if ((zon = real_zone(Z_KINGS_C)) == NOWHERE)
    return NOBODY;

  return zone_table[zon].bot + offset;
}

static room_rnum castle_real_room(room_vnum roomoffset)
{
  zone_rnum zon;

  if ((zon = real_zone(Z_KINGS_C)) == NOWHERE)
    return NOWHERE;

  return real_room(zone_table[zon].bot + roomoffset);
}

/* Routine: assign_kings_castle. Used to assign function pointers to all mobiles
 * in the Kings Castle. Called from spec_assign.c. */
void assign_kings_castle(void)
{
  castle_mob_spec(0, CastleGuard);	/* Gwydion */
  castle_mob_spec(1, king_welmar);	/* Our dear friend, the King */
  castle_mob_spec(3, CastleGuard);	/* Jim */
  castle_mob_spec(4, CastleGuard);	/* Brian */
  castle_mob_spec(5, CastleGuard);	/* Mick */
  castle_mob_spec(6, CastleGuard);	/* Matt */
  castle_mob_spec(7, CastleGuard);	/* Jochem */
  castle_mob_spec(8, CastleGuard);	/* Anne */
  castle_mob_spec(9, CastleGuard);	/* Andrew */
  castle_mob_spec(10, CastleGuard);	/* Bertram */
  castle_mob_spec(11, CastleGuard);	/* Jeanette */
  castle_mob_spec(12, peter);		/* Peter */
  castle_mob_spec(13, training_master);	/* The training master */
  castle_mob_spec(16, James);		/* James the Butler */
  castle_mob_spec(17, cleaning);	/* Ze Cleaning Fomen */
  castle_mob_spec(20, tim);		/* Tim, Tom's twin */
  castle_mob_spec(21, tom);		/* Tom, Tim's twin */
  castle_mob_spec(24, DicknDavid);	/* Dick, guard of the Treasury */
  castle_mob_spec(25, DicknDavid);	/* David, Dicks brother */
  castle_mob_spec(26, jerry);		/* Jerry, the Gambler */
  castle_mob_spec(27, CastleGuard);	/* Michael */
  castle_mob_spec(28, CastleGuard);	/* Hans */
  castle_mob_spec(29, CastleGuard);	/* Boris */
}

/* Routine: member_of_staff. Used to see if a character is a member of the 
 * castle staff. Used mainly by BANZAI:ng NPC:s. */
static int member_of_staff(struct char_data *chChar)
{
  int ch_num;

  if (!IS_NPC(chChar))
    return (FALSE);

  ch_num = GET_MOB_VNUM(chChar);

  if (ch_num == castle_virtual(1))
    return (TRUE);

  if (ch_num > castle_virtual(2) && ch_num < castle_virtual(15))
    return (TRUE);

  if (ch_num > castle_virtual(15) && ch_num < castle_virtual(18))
    return (TRUE);

  if (ch_num > castle_virtual(18) && ch_num < castle_virtual(30))
    return (TRUE);

  return (FALSE);
}

/* Function: member_of_royal_guard. Returns TRUE if the character is a guard on
 * duty, otherwise FALSE. Used by Peter the captain of the royal guard. */
static int member_of_royal_guard(struct char_data *chChar)
{
  int ch_num;

  if (!chChar || !IS_NPC(chChar))
    return (FALSE);

  ch_num = GET_MOB_VNUM(chChar);

  if (ch_num == castle_virtual(3) || ch_num == castle_virtual(6))
    return (TRUE);

  if (ch_num > castle_virtual(7) && ch_num < castle_virtual(12))
    return (TRUE);

  if (ch_num > castle_virtual(23) && ch_num < castle_virtual(26))
    return (TRUE);

  return (FALSE);
}

/* Function: find_npc_by_name. Returns a pointer to an npc by the given name.
 * Used by Tim and Tom. */
static struct char_data *find_npc_by_name(struct char_data *chAtChar,
		const char *pszName, int iLen)
{
  struct char_data *ch;

  for (ch = world[IN_ROOM(chAtChar)].people; ch; ch = ch->next_in_room)
    if (IS_NPC(ch) && !strncmp(pszName, ch->player.short_descr, iLen))
      return (ch);

  return (NULL);
}

/* Function: find_guard. Returns the pointer to a guard on duty. Used by Peter 
 * the Captain of the Royal Guard */
static struct char_data *find_guard(struct char_data *chAtChar)
{
  struct char_data *ch;

  for (ch = world[IN_ROOM(chAtChar)].people; ch; ch = ch->next_in_room)
    if (!FIGHTING(ch) && member_of_royal_guard(ch))
      return (ch);

  return (NULL);
}

/* Function: get_victim. Returns a pointer to a randomly chosen character in 
 * the same room, fighting someone in the castle staff. Used by BANZAII-ing 
 * characters and King Welmar... */
static struct char_data *get_victim(struct char_data *chAtChar)
{
  struct char_data *ch;
  int iNum_bad_guys = 0, iVictim;

  for (ch = world[IN_ROOM(chAtChar)].people; ch; ch = ch->next_in_room)
    if (FIGHTING(ch) && member_of_staff(FIGHTING(ch)))
      iNum_bad_guys++;

  if (!iNum_bad_guys)
    return (NULL);

  iVictim = rand_number(0, iNum_bad_guys);	/* How nice, we give them a chance */
  if (!iVictim)
    return (NULL);

  iNum_bad_guys = 0;

  for (ch = world[IN_ROOM(chAtChar)].people; ch; ch = ch->next_in_room) {
    if (FIGHTING(ch) == NULL)
      continue;

    if (!member_of_staff(FIGHTING(ch)))
      continue;

    if (++iNum_bad_guys != iVictim)
      continue;

    return (ch);
  }

  return (NULL);
}

/* Banzaii. Makes a character banzaii on attackers of the castle staff. Used 
 * by Guards, Tim, Tom, Dick, David, Peter, Master, and the King. */
static int banzaii(struct char_data *ch)
{
  struct char_data *chOpponent;

  if (!AWAKE(ch) || FIGHTING(ch) || !(chOpponent = get_victim(ch)))
    return (FALSE);

  act("$n roars: 'Protect the Kingdom of Great King Welmar!  BANZAIIII!!!'",
	FALSE, ch, 0, 0, TO_ROOM);
  hit(ch, chOpponent, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
  return (TRUE);
}

/* Do_npc_rescue. Makes ch_hero rescue ch_victim. Used by Tim and Tom. */
int do_npc_rescue(struct char_data *ch_hero, struct char_data *ch_victim)
{
  struct char_data *ch_bad_guy;

  for (ch_bad_guy = world[IN_ROOM(ch_hero)].people;
       ch_bad_guy && (FIGHTING(ch_bad_guy) != ch_victim);
       ch_bad_guy = ch_bad_guy->next_in_room);

  /* NO WAY I'll rescue the one I'm fighting! */
  if (!ch_bad_guy || ch_bad_guy == ch_hero)
    return (FALSE);

  act("You bravely rescue $N.\r\n", FALSE, ch_hero, 0, ch_victim, TO_CHAR);
  act("You are rescued by $N!\r\n",
	FALSE, ch_victim, 0, ch_hero, TO_CHAR);
  act("$n heroically rescues $N.", FALSE, ch_hero, 0, ch_victim, TO_NOTVICT);

  if (FIGHTING(ch_bad_guy))
    stop_fighting(ch_bad_guy);
  if (FIGHTING(ch_hero))
    stop_fighting(ch_hero);

  set_fighting(ch_hero, ch_bad_guy);
  set_fighting(ch_bad_guy, ch_hero);
  return (TRUE);
}

/* Procedure to block a person trying to enter a room. Used by Tim/Tom at Kings 
 * bedroom and Dick/David at treasury. */
static int block_way(struct char_data *ch, int cmd, char *arg, room_vnum iIn_room,
	          int iProhibited_direction)
{
  if (cmd != ++iProhibited_direction)
    return (FALSE);

  if (ch->player.short_descr && !strncmp(ch->player.short_descr, "King Welmar", 11))
    return (FALSE);

  if (IN_ROOM(ch) != real_room(iIn_room))
    return (FALSE);

  if (!member_of_staff(ch))
    act("The guard roars at $n and pushes $m back.", FALSE, ch, 0, 0, TO_ROOM);

  send_to_char(ch, "The guard roars: 'Entrance is Prohibited!', and pushes you back.\r\n");
  return (TRUE);
}

/* Routine to check if an object is trash. Used by James the Butler and the 
 * Cleaning Lady. */
static int is_trash(struct obj_data *i)
{
  if (!OBJWEAR_FLAGGED(i, ITEM_WEAR_TAKE))
    return (FALSE);

  if (GET_OBJ_TYPE(i) == ITEM_DRINKCON || GET_OBJ_COST(i) <= 10)
    return (TRUE);

  return (FALSE);
}

/* Fry_victim. Finds a suitabe victim, and cast some _NASTY_ spell on him. Used 
 * by King Welmar. */
static void fry_victim(struct char_data *ch)
{
  struct char_data *tch;

  if (ch->points.mana < 10)
    return;

  /* Find someone suitable to fry ! */
  if (!(tch = get_victim(ch)))
    return;

  switch (rand_number(0, 8)) {
  case 1:
  case 2:
  case 3:
    send_to_char(ch, "You raise your hand in a dramatical gesture.\r\n");
    act("$n raises $s hand in a dramatical gesture.", 1, ch, 0, 0, TO_ROOM);
    cast_spell(ch, tch, 0, SPELL_COLOR_SPRAY);
    break;
  case 4:
  case 5:
    send_to_char(ch, "You concentrate and mumble to yourself.\r\n");
    act("$n concentrates, and mumbles to $mself.", 1, ch, 0, 0, TO_ROOM);
    cast_spell(ch, tch, 0, SPELL_HARM);
    break;
  case 6:
  case 7:
    act("You look deeply into the eyes of $N.", 1, ch, 0, tch, TO_CHAR);
    act("$n looks deeply into the eyes of $N.", 1, ch, 0, tch, TO_NOTVICT);
    act("You see an ill-boding flame in the eye of $n.", 1, ch, 0, tch, TO_VICT);
    cast_spell(ch, tch, 0, SPELL_FIREBALL);
    break;
  default:
    if (!rand_number(0, 1))
      cast_spell(ch, ch, 0, SPELL_HEAL);
    break;
  }

  ch->points.mana -= 10;

  return;
}

/* King_welmar. Control the actions and movements of the King. */
SPECIAL(king_welmar)
{
  char actbuf[MAX_INPUT_LENGTH] = { '\0' };

  const char *monolog[] = {
    "$n proclaims 'Primus in regnis Geticis coronam'.",
    "$n proclaims 'regiam gessi, subiique regis'.",
    "$n proclaims 'munus et mores colui sereno'.",
    "$n proclaims 'principe dignos'."
  };

  const char bedroom_path[] = "s33004o1c1S.";
  const char throne_path[] = "W3o3cG52211rg.";
  const char monolog_path[] = "ABCDPPPP.";

  static const char *path = NULL;
  static int path_index = 0;
  static bool move = FALSE;

  if (!move) {
    if (time_info.hours == 8 && IN_ROOM(ch) == castle_real_room(51)) {
      move = TRUE;
      path = throne_path;
      path_index = 0;
    } else if (time_info.hours == 21 && IN_ROOM(ch) == castle_real_room(17)) {
      move = TRUE;
      path = bedroom_path;
      path_index = 0;
    } else if (time_info.hours == 12 && IN_ROOM(ch) == castle_real_room(17)) {
      move = TRUE;
      path = monolog_path;
      path_index = 0;
    }
  }
  if (cmd || (GET_POS(ch) < POS_SLEEPING) ||
      (GET_POS(ch) == POS_SLEEPING && !move))
    return (FALSE);

  if (FIGHTING(ch)) {
    fry_victim(ch);
    return (FALSE);
  } else if (banzaii(ch))
    return (FALSE);

  if (!move)
    return (FALSE);

  switch (path[path_index]) {
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
    perform_move(ch, path[path_index] - '0', 1);
    break;

  case 'A':
  case 'B':
  case 'C':
  case 'D':
    act(monolog[path[path_index] - 'A'], FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'P':
    break;

  case 'W':
    GET_POS(ch) = POS_STANDING;
    act("$n awakens and stands up.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'S':
    GET_POS(ch) = POS_SLEEPING;
    act("$n lies down on $s beautiful bed and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'r':
    GET_POS(ch) = POS_SITTING;
    act("$n sits down on $s great throne.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 's':
    GET_POS(ch) = POS_STANDING;
    act("$n stands up.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'G':
    act("$n says 'Good morning, trusted friends.'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'g':
    act("$n says 'Good morning, dear subjects.'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'o':
    do_gen_door(ch, strcpy(actbuf, "door"), 0, SCMD_UNLOCK);	/* strcpy: OK */
    do_gen_door(ch, strcpy(actbuf, "door"), 0, SCMD_OPEN);	/* strcpy: OK */
    break;

  case 'c':
    do_gen_door(ch, strcpy(actbuf, "door"), 0, SCMD_CLOSE);	/* strcpy: OK */
    do_gen_door(ch, strcpy(actbuf, "door"), 0, SCMD_LOCK);	/* strcpy: OK */
    break;

  case '.':
    move = FALSE;
    break;
  }

  path_index++;
  return (FALSE);
}

/* Training_master. Acts actions to the training room, if his students are 
 * present. Also allowes warrior-class to practice. Used by the Training 
 * Master. */
SPECIAL(training_master)
{
  struct char_data *pupil1, *pupil2 = NULL, *tch;

  if (!AWAKE(ch) || FIGHTING(ch))
    return (FALSE);

  if (cmd)
    return (FALSE);

  if (banzaii(ch) || rand_number(0, 2))
    return (FALSE);

  if (!(pupil1 = find_npc_by_name(ch, "Brian", 5)))
    return (FALSE);

  if (!(pupil2 = find_npc_by_name(ch, "Mick", 4)))
    return (FALSE);

  if (FIGHTING(pupil1) || FIGHTING(pupil2))
    return (FALSE);

  if (rand_number(0, 1)) {
    tch = pupil1;
    pupil1 = pupil2;
    pupil2 = tch;
  }

  switch (rand_number(0, 7)) {
  case 0:
    act("$n hits $N on $s head with a powerful blow.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
    act("You hit $N on $s head with a powerful blow.", FALSE, pupil1, 0, pupil2, TO_CHAR);
    act("$n hits you on your head with a powerful blow.", FALSE, pupil1, 0, pupil2, TO_VICT);
    break;

  case 1:
    act("$n hits $N in $s chest with a thrust.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
    act("You manage to thrust $N in the chest.", FALSE, pupil1, 0, pupil2, TO_CHAR);
    act("$n manages to thrust you in your chest.", FALSE, pupil1, 0, pupil2, TO_VICT);
    break;

  case 2:
    send_to_char(ch, "You command your pupils to bow.\r\n");
    act("$n commands $s pupils to bow.", FALSE, ch, 0, 0, TO_ROOM);
    act("$n bows before $N.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
    act("$N bows before $n.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
    act("You bow before $N, who returns your gesture.", FALSE, pupil1, 0, pupil2, TO_CHAR);
    act("You bow before $n, who returns your gesture.", FALSE, pupil1, 0, pupil2, TO_VICT);
    break;

  case 3:
    act("$N yells at $n, as he fumbles and drops $s sword.", FALSE, pupil1, 0, ch, TO_NOTVICT);
    act("$n quickly picks up $s weapon.", FALSE, pupil1, 0, 0, TO_ROOM);
    act("$N yells at you, as you fumble, losing your weapon.", FALSE, pupil1, 0, ch, TO_CHAR);
    send_to_char(pupil1, "You quickly pick up your weapon again.\r\n");
    act("You yell at $n, as he fumbles, losing $s weapon.", FALSE, pupil1, 0, ch, TO_VICT);
    break;

  case 4:
    act("$N tricks $n, and slashes him across the back.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
    act("$N tricks you, and slashes you across your back.", FALSE, pupil1, 0, pupil2, TO_CHAR);
    act("You trick $n, and quickly slash him across $s back.", FALSE, pupil1, 0, pupil2, TO_VICT);
    break;

  case 5:
    act("$n lunges a blow at $N but $N parries skillfully.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
    act("You lunge a blow at $N but $E parries skillfully.", FALSE, pupil1, 0, pupil2, TO_CHAR);
    act("$n lunges a blow at you, but you skillfully parry it.", FALSE, pupil1, 0, pupil2, TO_VICT);
    break;

  case 6:
    act("$n clumsily tries to kick $N, but misses.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
    act("You clumsily miss $N with your poor excuse for a kick.", FALSE, pupil1, 0, pupil2, TO_CHAR);
    act("$n fails an unusually clumsy attempt at kicking you.", FALSE, pupil1, 0, pupil2, TO_VICT);
    break;

  default:
    send_to_char(ch, "You show your pupils an advanced technique.\r\n");
    act("$n shows $s pupils an advanced technique.", FALSE, ch, 0, 0, TO_ROOM);
    break;
  }

  return (FALSE);
}

SPECIAL(tom)
{
  return castle_twin_proc(ch, cmd, argument, 48, "Tim");
}

SPECIAL(tim)
{
  return castle_twin_proc(ch, cmd, argument, 49, "Tom");
}

/* Common routine for the Castle Twins. */
static int castle_twin_proc(struct char_data *ch, int cmd, char *arg, int ctlnum, const char *twinname)
{
  struct char_data *king, *twin;

  if (!AWAKE(ch))
    return (FALSE);

  if (cmd)
    return block_way(ch, cmd, arg, castle_virtual(ctlnum), 1);

  if ((king = find_npc_by_name(ch, "King Welmar", 11)) != NULL) {
    char actbuf[MAX_INPUT_LENGTH];

    if (!ch->master)
      do_follow(ch, strcpy(actbuf, "King Welmar"), 0, 0);	/* strcpy: OK */
    if (FIGHTING(king))
      do_npc_rescue(ch, king);
  }

  if ((twin = find_npc_by_name(ch, twinname, strlen(twinname))) != NULL)
    if (FIGHTING(twin) && 2 * GET_HIT(twin) < GET_HIT(ch))
      do_npc_rescue(ch, twin);

  if (!FIGHTING(ch))
    banzaii(ch);

  return (FALSE);
}


/* Routine for James the Butler. Complains if he finds any trash. This doesn't 
 * make sure he _can_ carry it. */
SPECIAL(James)
{
  return castle_cleaner(ch, cmd, TRUE);
}

/* Common code for James and the Cleaning Woman. */
static int castle_cleaner(struct char_data *ch, int cmd, int gripe)
{
  struct obj_data *i;

  if (cmd || !AWAKE(ch) || FIGHTING(ch))
    return (FALSE);

  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
    if (!is_trash(i))
      continue;

    if (gripe) {
      act("$n says: 'My oh my!  I ought to fire that lazy cleaning woman!'",
          FALSE, ch, 0, 0, TO_ROOM);
      act("$n picks up a piece of trash.", FALSE, ch, 0, 0, TO_ROOM);
    }
    obj_from_room(i);
    obj_to_char(i, ch);
    return (TRUE);
  }

  return (FALSE);
}

/* Routine for the Cleaning Woman. Picks up any trash she finds. */
SPECIAL(cleaning)
{
  return castle_cleaner(ch, cmd, FALSE);
}

/* CastleGuard. Standard routine for ordinary castle guards. */
SPECIAL(CastleGuard)
{
  if (cmd || !AWAKE(ch) || FIGHTING(ch))
    return (FALSE);

  return (banzaii(ch));
}

/* DicknDave. Routine for the guards Dick and David. */
SPECIAL(DicknDavid)
{
  if (!AWAKE(ch))
    return (FALSE);

  if (!cmd && !FIGHTING(ch))
    banzaii(ch);

  return (block_way(ch, cmd, argument, castle_virtual(36), 1));
}

/*Peter. Routine for Captain of the Guards. */
SPECIAL(peter)
{
  struct char_data *ch_guard = NULL;

  if (cmd || !AWAKE(ch) || FIGHTING(ch))
    return (FALSE);

  if (banzaii(ch))
    return (FALSE);

  if (!(rand_number(0, 3)) && (ch_guard = find_guard(ch)))
    switch (rand_number(0, 5)) {
    case 0:
      act("$N comes sharply into attention as $n inspects $M.",
	  FALSE, ch, 0, ch_guard, TO_NOTVICT);
      act("$N comes sharply into attention as you inspect $M.",
	  FALSE, ch, 0, ch_guard, TO_CHAR);
      act("You go sharply into attention as $n inspects you.",
	  FALSE, ch, 0, ch_guard, TO_VICT);
      break;
    case 1:
      act("$N looks very small, as $n roars at $M.",
	  FALSE, ch, 0, ch_guard, TO_NOTVICT);
      act("$N looks very small as you roar at $M.",
	  FALSE, ch, 0, ch_guard, TO_CHAR);
      act("You feel very small as $N roars at you.",
	  FALSE, ch, 0, ch_guard, TO_VICT);
      break;
    case 2:
      act("$n gives $N some Royal directions.",
	  FALSE, ch, 0, ch_guard, TO_NOTVICT);
      act("You give $N some Royal directions.",
	  FALSE, ch, 0, ch_guard, TO_CHAR);
      act("$n gives you some Royal directions.",
	  FALSE, ch, 0, ch_guard, TO_VICT);
      break;
    case 3:
      act("$n looks at you.", FALSE, ch, 0, ch_guard, TO_VICT);
      act("$n looks at $N.", FALSE, ch, 0, ch_guard, TO_NOTVICT);
      act("$n growls: 'Those boots need polishing!'",
	  FALSE, ch, 0, ch_guard, TO_ROOM);
      act("You growl at $N.", FALSE, ch, 0, ch_guard, TO_CHAR);
      break;
    case 4:
      act("$n looks at you.", FALSE, ch, 0, ch_guard, TO_VICT);
      act("$n looks at $N.", FALSE, ch, 0, ch_guard, TO_NOTVICT);
      act("$n growls: 'Straighten that collar!'",
	  FALSE, ch, 0, ch_guard, TO_ROOM);
      act("You growl at $N.", FALSE, ch, 0, ch_guard, TO_CHAR);
      break;
    default:
      act("$n looks at you.", FALSE, ch, 0, ch_guard, TO_VICT);
      act("$n looks at $N.", FALSE, ch, 0, ch_guard, TO_NOTVICT);
      act("$n growls: 'That chain mail looks rusty!  CLEAN IT !!!'",
	  FALSE, ch, 0, ch_guard, TO_ROOM);
      act("You growl at $N.", FALSE, ch, 0, ch_guard, TO_CHAR);
      break;
    }

  return (FALSE);
}

/* Procedure for Jerry and Michael in x08 of King's Castle. Code by Sapowox 
 * modified by Pjotr.(Original code from Master) */
SPECIAL(jerry)
{
  struct char_data *gambler1, *gambler2 = NULL, *tch;

  if (!AWAKE(ch) || FIGHTING(ch))
    return (FALSE);

  if (cmd)
    return (FALSE);

  if (banzaii(ch) || rand_number(0, 2))
    return (FALSE);

  if (!(gambler1 = ch))
    return (FALSE);

  if (!(gambler2 = find_npc_by_name(ch, "Michael", 7)))
    return (FALSE);

  if (FIGHTING(gambler1) || FIGHTING(gambler2))
    return (FALSE);

  if (rand_number(0, 1)) {
    tch = gambler1;
    gambler1 = gambler2;
    gambler2 = tch;
  }

  switch (rand_number(0, 5)) {
  case 0:
    act("$n rolls the dice and cheers loudly at the result.",
	    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
    act("You roll the dice and cheer. GREAT!",
	    FALSE, gambler1, 0, gambler2, TO_CHAR);
    act("$n cheers loudly as $e rolls the dice.",
	    FALSE, gambler1, 0, gambler2, TO_VICT);
    break;
  case 1:
    act("$n curses the Goddess of Luck roundly as he sees $N's roll.",
	    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
    act("You curse the Goddess of Luck as $N rolls.",
	    FALSE, gambler1, 0, gambler2, TO_CHAR);
    act("$n swears angrily. You are in luck!",
	    FALSE, gambler1, 0, gambler2, TO_VICT);
    break;
  case 2:
    act("$n sighs loudly and gives $N some gold.",
	    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
    act("You sigh loudly at the pain of having to give $N some gold.",
	    FALSE, gambler1, 0, gambler2, TO_CHAR);
    act("$n sighs loudly as $e gives you your rightful win.",
	    FALSE, gambler1, 0, gambler2, TO_VICT);
    break;
  case 3:
    act("$n smiles remorsefully as $N's roll tops $s.",
	    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
    act("You smile sadly as you see that $N beats you. Again.",
	    FALSE, gambler1, 0, gambler2, TO_CHAR);
    act("$n smiles remorsefully as your roll tops $s.",
	    FALSE, gambler1, 0, gambler2, TO_VICT);
    break;
  case 4:
    act("$n excitedly follows the dice with $s eyes.",
	    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
    act("You excitedly follow the dice with your eyes.",
	    FALSE, gambler1, 0, gambler2, TO_CHAR);
    act("$n excitedly follows the dice with $s eyes.",
	    FALSE, gambler1, 0, gambler2, TO_VICT);
    break;
  default:
    act("$n says 'Well, my luck has to change soon', as he shakes the dice.",
	    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
    act("You say 'Well, my luck has to change soon' and shake the dice.",
	    FALSE, gambler1, 0, gambler2, TO_CHAR);
    act("$n says 'Well, my luck has to change soon', as he shakes the dice.",
	    FALSE, gambler1, 0, gambler2, TO_VICT);
    break;
  }
  return (FALSE);
}

/******************************************************************/
/*  END KINGS CASTLE */
/******************************************************************/

/*********/
/* ABYSS */
/*********/

#define ZONE_VNUM   1423

/* just made this to help facilitate switching of zone vnums if needed */
int calc_room_num(int value) {
  return (ZONE_VNUM * 100) + value;
}

/* this proc swaps exits in the rooms in a given area */
SPECIAL(abyss_randomizer) {
  struct char_data *i = NULL;
  char buf[MAX_INPUT_LENGTH] = { '\0' };

  if (cmd)
    return 0;

  if (rand_number(0, 9))
    return 0;

  int room, temp1, temp2;

  for (room = calc_room_num(1); room <= calc_room_num(18); room++) {

    /* Swapping North and South */
    if (world[real_room(room)].dir_option[NORTH] && world[real_room(room)].dir_option[NORTH]->to_room != NOWHERE)
      temp1 = world[real_room(room)].dir_option[NORTH]->to_room;
    else
      temp1 = NOWHERE;
    if (world[real_room(room)].dir_option[SOUTH] && world[real_room(room)].dir_option[SOUTH]->to_room != NOWHERE)
      temp2 = world[real_room(room)].dir_option[SOUTH]->to_room;
    else
      temp2 = NOWHERE;
    if (temp2 != NOWHERE) {
      if (!world[real_room(room)].dir_option[NORTH])
        CREATE(world[real_room(room)].dir_option[NORTH], struct room_direction_data, 1);
      world[real_room(room)].dir_option[NORTH]->to_room = temp2;
    } else if (world[real_room(room)].dir_option[NORTH]) {
      free(world[real_room(room)].dir_option[NORTH]);
      world[real_room(room)].dir_option[NORTH] = NULL;
    }
    if (temp1 != NOWHERE) {
      if (!world[real_room(room)].dir_option[SOUTH])
        CREATE(world[real_room(room)].dir_option[SOUTH], struct room_direction_data, 1);
      world[real_room(room)].dir_option[SOUTH]->to_room = temp1;
    } else if (world[real_room(room)].dir_option[SOUTH]) {
      free(world[real_room(room)].dir_option[SOUTH]);
      world[real_room(room)].dir_option[SOUTH] = NULL;
    }

    /* Swapping East and West */
    if (world[real_room(room)].dir_option[EAST] && world[real_room(room)].dir_option[EAST]->to_room != NOWHERE)
      temp1 = world[real_room(room)].dir_option[EAST]->to_room;
    else
      temp1 = NOWHERE;
    if (world[real_room(room)].dir_option[WEST] && world[real_room(room)].dir_option[WEST]->to_room != NOWHERE)
      temp2 = world[real_room(room)].dir_option[WEST]->to_room;
    else
      temp2 = NOWHERE;
    if (temp2 != NOWHERE) {
      if (!world[real_room(room)].dir_option[EAST])
        CREATE(world[real_room(room)].dir_option[EAST], struct room_direction_data, 1);
      world[real_room(room)].dir_option[EAST]->to_room = temp2;
    } else if (world[real_room(room)].dir_option[EAST]) {
      free(world[real_room(room)].dir_option[EAST]);
      world[real_room(room)].dir_option[EAST] = NULL;
    }
    if (temp1 != NOWHERE) {
      if (!world[real_room(room)].dir_option[WEST])
        CREATE(world[real_room(room)].dir_option[WEST], struct room_direction_data, 1);
      world[real_room(room)].dir_option[WEST]->to_room = temp1;
    } else if (world[real_room(room)].dir_option[WEST]) {
      free(world[real_room(room)].dir_option[WEST]);
      world[real_room(room)].dir_option[WEST] = NULL;
    }

    /* Swapping Up and Down */
    if (world[real_room(room)].dir_option[UP] && world[real_room(room)].dir_option[UP]->to_room != NOWHERE)
      temp1 = world[real_room(room)].dir_option[UP]->to_room;
    else
      temp1 = NOWHERE;
    if (world[real_room(room)].dir_option[DOWN] && world[real_room(room)].dir_option[DOWN]->to_room != NOWHERE)
      temp2 = world[real_room(room)].dir_option[DOWN]->to_room;
    else
      temp2 = NOWHERE;
    if (temp2 != NOWHERE) {
      if (!world[real_room(room)].dir_option[UP])
        CREATE(world[real_room(room)].dir_option[UP], struct room_direction_data, 1);
      world[real_room(room)].dir_option[UP]->to_room = temp2;
    } else if (world[real_room(room)].dir_option[UP]) {
      free(world[real_room(room)].dir_option[UP]);
      world[real_room(room)].dir_option[UP] = NULL;
    }
    if (temp1 != NOWHERE) {
      if (!world[real_room(room)].dir_option[DOWN])
        CREATE(world[real_room(room)].dir_option[DOWN], struct room_direction_data, 1);
      world[real_room(room)].dir_option[DOWN]->to_room = temp1;
    } else if (world[real_room(room)].dir_option[DOWN]) {
      free(world[real_room(room)].dir_option[DOWN]);
      world[real_room(room)].dir_option[DOWN] = NULL;
    }
  }

  sprintf(buf, "\tpThe world seems to shift.\tn\r\n");

  for (i = character_list; i; i = i->next)
    if (world[ch->in_room].zone == world[i->in_room].zone)
      send_to_char(i, buf);

  return 0;
}

#undef ZONE_VNUM

/*************/
/* End Abyss */
/*************/

/*****************/
/* Crimson Flame */
/*****************/

#define CF_VNUM   1060

/* just made this to help facilitate switching of zone vnums if needed */
int cf_converter(int value) {
  return (CF_VNUM * 100) + value;
}

/* this proc will cause the training master to sick all his minions to track
   whoever he is fighting - will fire one time and that's it */
SPECIAL(cf_trainingmaster) {
  struct char_data *i = NULL;
  struct char_data *enemy = NULL;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return 0;

  enemy = FIGHTING(ch);

  if (!enemy)
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) {
    if (enemy->master && enemy->master->in_room == enemy->in_room)
      enemy = enemy->master;
    act("$n waves $s hand slightly.", FALSE, ch, 0, 0, TO_ROOM);
    for (i = character_list; i; i = i->next) {
      if (!FIGHTING(i) && IS_NPC(i) && (GET_MOB_VNUM(i) == cf_converter(32) ||
              GET_MOB_VNUM(i) == cf_converter(33) || GET_MOB_VNUM(i) == cf_converter(34) ||
              GET_MOB_VNUM(i) == cf_converter(35) || GET_MOB_VNUM(i) == cf_converter(36) ||
              GET_MOB_VNUM(i) == cf_converter(37) || GET_MOB_VNUM(i) == cf_converter(38) ||
              GET_MOB_VNUM(i) == cf_converter(39)) && ch != i) {
        if (ch->in_room != i->in_room) {
          HUNTING(i) = enemy;
          hunt_victim(i);
        } else
          hit(ch, enemy, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }
    }  // for loop
    
    PROC_FIRED(ch) = TRUE;
    return 1;
  }
  
  return 0;
}

/* this is lord alathar's proc to summon his bodyguards to him */
SPECIAL(cf_alathar) {
  struct char_data *mob = NULL;
  int i = 0;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return 0;

  if (!FIGHTING(ch))
    return 0;

  if (PROC_FIRED(ch))
    return FALSE;

  send_to_room(IN_ROOM(ch),
          "\tDLord Alathar thrusts his hands out and makes a sweeping gesture\tn\r\n"
          "\tDwhile uttering words in an unknown tongue. The \tRcrimson colored\tn\r\n"
          "\tRflames \tDin the huge black brazier blaze even brighter than before\tn\r\n"
          "\tDproducing a great and \tRblinding red radiance \tDthroughout the\tn\r\n"
          "\tDarea. Dark shadows are summoned and swirl into view then swarm to\tn\r\n"
          "\tDLord Alathar's aid.\tn");

  if (!GROUP(ch))
    create_group(ch);
      
  for (i = 50; i < 57; i++) {
    mob = read_mobile(cf_converter(i), VIRTUAL);
    if (mob) {
      char_to_room(mob, ch->in_room);
      add_follower(mob, ch);
      join_group(mob, GROUP(ch));
    }
  }

  PROC_FIRED(ch) = TRUE;

  return TRUE;
}   

#undef CF_VNUM

/*********************/
/* End Crimson Flame */
/*********************/

/*****************/
/* Jot           */
/*****************/

#define JOT_VNUM   1960

#define MAX_FG 60  // fire giants
#define MAX_SB 20  // smoking beard batallion
#define MAX_EM 20  // efreeti mercenaries
#define MAX_FROST 65  // frost giants

bool jot_inv_check = false;

ACMD(do_say);


/* just made this to help facilitate switching of zone vnums if needed */
int jot_converter(int value) {
  return (JOT_VNUM * 100) + value;
}

/* currently unused */
void jot_invasion() {
  if (jot_inv_check)
    return;
  
  jot_inv_check = true;
  
  if (rand_number(0, 99) <= 2)
    return;
}

/* load rooms for fire giants */
int fg_pos[MAX_FG] = {
  295, 295, 295, 295, 295,
  295, 295, 295, 295, 295,
  295, 295, 295, 295, 295,
  295, 295, 295, 295, 295,
  295, 295, 295, 295, 295,
  295, 295, 295, 295, 295,
  295, 295, 295, 295, 295,
  295, 295, 295, 295, 295,
  215, 215, 215, 215, 215,
  212, 218, 222, 207, 188,
  204, 204, 204, 204, 196,
  204, 204, 204, 204, 196
};

/* load rooms for smoking beard batallion */
int sb_pos[MAX_SB] = {
  295, 295, 295, 295, 295, 
  295, 295, 295, 295, 295, 
  295, 295, 295, 215, 215, 
  188, 188, 217, 206, 206
};

/* load rooms for frost giants */
int frost_pos[MAX_FROST] = {
  286, 286, 282, 283, 284,
  285, 285, 285, 286, 286,
  273, 273, 270, 270, 269,
  273, 273, 270, 270, 269,
  266, 266, 267, 264, 264,
  266, 266, 267, 264, 264,
  265, 272, 272, 271, 271,
  228, 240, 240, 233, 233,
  233, 235, 235, 235, 251,
  251, 252, 252, 253, 253,
  251, 252, 252, 253, 253,
  244, 244, 255, 255, 254,
  254, 256, 256, 243, 243
};


/* spec proc for loading the jot invasion */
SPECIAL(jot_invasion_loader) {
  struct char_data *tch = NULL, *chmove = NULL, 
          *glammad = NULL, *leader = NULL, *mob = NULL;
  int i = 0;
  int where = -1;
  struct obj_data *obj = NULL, *obj2 = NULL;
  obj_rnum objrnum = NOTHING;
  room_rnum roomrnum = NOWHERE;
  mob_rnum mobrnum = NOWHERE;

  if (cmd || PROC_FIRED(ch) == TRUE)
    return 0;

  /* moving these special mobiles from their storage room to jot */
  for (tch = world[ch->in_room].people; tch; tch = chmove) {
    chmove = tch->next_in_room;
    /* glammad */
    if (GET_MOB_VNUM(tch) == jot_converter(80)) {
      if ((roomrnum = real_room(jot_converter(204))) != NOWHERE) {
        glammad = tch;  /* going to use this to form a group */
        char_from_room(glammad);
        char_to_room(glammad, roomrnum);
        if (!GROUP(glammad))
          create_group(glammad);
      }
    }
    /* fire giant captain(s) */
    if (GET_MOB_VNUM(tch) == jot_converter(81)) {
      if ((roomrnum = real_room(jot_converter(204))) != NOWHERE) {
        char_from_room(tch);
        char_to_room(tch, roomrnum);
      }
    }
    /* sirthon quilen */
    if (GET_MOB_VNUM(tch) == jot_converter(83)) {
      if ((roomrnum = real_room(jot_converter(115))) != NOWHERE) {
        char_from_room(tch);
        char_to_room(tch, roomrnum);
      }
    }
  }
  
  /* soldiers to glammad */
  for (i = 0; i < 2; i++) {
    mob = read_mobile(jot_converter(78), VIRTUAL);
    obj = read_object(jot_converter(17), VIRTUAL);
    if (obj && mob) {
      obj_to_char(obj, mob);
      perform_wield(mob, obj, TRUE);
      if ((roomrnum = real_room(jot_converter(204))) != NOWHERE) {
        char_to_room(mob, roomrnum);
        SET_BIT_AR(MOB_FLAGS(mob), MOB_SENTINEL);
        REMOVE_BIT_AR(MOB_FLAGS(mob), MOB_LISTEN);
        if (glammad) {
          add_follower(mob, glammad);
          join_group(mob, GROUP(glammad));
        }
      }
    }
  }
  
  /* twilight to treasure room */
  if ((objrnum = real_object(jot_converter(90))) != NOWHERE) {
    if ((obj = read_object(objrnum, REAL)) != NULL) {
      if ((roomrnum = real_room(jot_converter(296))) != NOWHERE) {
        obj_to_room(obj, roomrnum);
      }
    }
  }
  /* fire giant crown to treasure room */
  if ((objrnum = real_object(jot_converter(82))) != NOWHERE) {
    if ((obj = read_object(objrnum, REAL)) != NULL) {
      if ((roomrnum = real_room(jot_converter(296))) != NOWHERE) {
        obj_to_room(obj, roomrnum);
      }
    }
  }

  /* extra jarls to deal with */
  for (i = 0; i < 2; i++) {  /* treasure room */
    if ((mobrnum = real_mobile(jot_converter(39))) != NOBODY) {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL) {
        if ((roomrnum = real_room(jot_converter(296))) != NOWHERE) {
          char_to_room(mob, roomrnum);
        }
      }
    }
  }
  for (i = 0; i < 3; i++) {  /* uthgard loki throne room */
    if ((mobrnum = real_mobile(jot_converter(39))) != NOBODY) {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL) {
        if ((roomrnum = real_room(jot_converter(287))) != NOWHERE) {
          char_to_room(mob, roomrnum);
        }
      }
    }
  }
  
  /* heavily guarded gatehouse, frost giant mage is leading this group */
  if ((mobrnum = real_mobile(jot_converter(90))) != NOBODY) {
    if ((leader = read_mobile(mobrnum, REAL)) != NULL) {
      if ((roomrnum = real_room(jot_converter(266))) != NOWHERE) {
        char_to_room(leader, roomrnum);
        if (!GROUP(leader))
          create_group(leader);
      }      
    }
  }
  /* 2nd mage in group */
  if ((mobrnum = real_mobile(jot_converter(90))) != NOBODY) {
    if ((mob = read_mobile(mobrnum, REAL)) != NULL) {
      if ((roomrnum = real_room(jot_converter(266))) != NOWHERE) {
        char_to_room(mob, roomrnum);
        if (leader) {
          add_follower(mob, leader);
          join_group(mob, GROUP(leader));
        }
      }      
    }
  }
  /* citadel guards join the group */
  if (roomrnum != NOWHERE) {
    for (i = 0; i < 8; i++) {
      mob = read_mobile(jot_converter(33), VIRTUAL);
      obj = read_object(jot_converter(28), VIRTUAL);
      if (mob && obj) {
        obj_to_char(obj, mob);
        perform_wield(mob, obj, TRUE);
      }
      if ((obj2 = read_object(jot_converter(41), VIRTUAL)) != NULL) {
        obj_to_char(obj2, mob);
        where = find_eq_pos(mob, obj2, 0);
        perform_wear(mob, obj2, where);
      }
      char_to_room(mob, roomrnum);
      if (leader) {
        add_follower(mob, leader);
        join_group(mob, GROUP(leader));
      }
    }
  }
  
  /* large gatehouse group, led by a mage again */
  if ((mobrnum = real_mobile(jot_converter(90))) != NOBODY) {
    if ((leader = read_mobile(mobrnum, REAL)) != NULL) {
      if ((roomrnum = real_room(jot_converter(252))) != NOWHERE) {
        char_to_room(leader, roomrnum);
        if (!GROUP(leader))
          create_group(leader);        
      }
    }
  }
  /* 2nd mage in group */
  if ((mobrnum = real_mobile(jot_converter(90))) != NOBODY) {
    if ((mob = read_mobile(mobrnum, REAL)) != NULL) {
      if ((roomrnum = real_room(jot_converter(252))) != NOWHERE) {
        char_to_room(mob, roomrnum);
        if (leader) {
          add_follower(mob, leader);
          join_group(mob, GROUP(leader));
        }
      }      
    }
  }
  /* citadel guards join the group */
  if (roomrnum != NOWHERE) {
    for (i = 0; i < 5; i++) {
      mob = read_mobile(jot_converter(33), VIRTUAL);
      obj = read_object(jot_converter(28), VIRTUAL);
      if (mob && obj) {
        obj_to_char(obj, mob);
        perform_wield(mob, obj, TRUE);
      }
      if ((obj2 = read_object(jot_converter(40), VIRTUAL)) != NULL) {
        obj_to_char(obj2, mob);
        where = find_eq_pos(mob, obj2, 0);
        perform_wear(mob, obj2, where);
      }
      char_to_room(mob, roomrnum);
      if (leader) {
        add_follower(mob, leader);
        join_group(mob, GROUP(leader));
      }
    }
  }

  /* load up some firegiants, then equip them */
  for (i = 0; i < MAX_FG; i++) {
    if ((roomrnum = real_room(jot_converter(fg_pos[i]))) != NOWHERE) {
      if ((mob = read_mobile(jot_converter(78), VIRTUAL)) != NULL) {
        if ((obj = read_object(jot_converter(17), VIRTUAL)) != NULL) {
          obj_to_char(obj, mob);
          perform_wield(mob, obj, TRUE);          
        }
        char_to_room(mob, roomrnum);
      }
    }
  }

  /* load up smoking beard batallion */
  for (i = 0; i < MAX_SB; i++) {
    if ((roomrnum = real_room(jot_converter(sb_pos[i]))) != NOWHERE) {
      if ((mob = read_mobile(jot_converter(79), VIRTUAL)) != NULL) {
        char_to_room(mob, roomrnum);
      }
    }
  }

  /* efreeti mercenary */
  for (i = 0; i < MAX_EM; i++) {
    if ((roomrnum = real_room(jot_converter(295))) != NOWHERE) {
      if ((mob = read_mobile(jot_converter(84), VIRTUAL)) != NULL) {
        char_to_room(mob, roomrnum);
      }
    }
  }

  /* Extra frost giants */
  for (i = 0; i < MAX_FROST; i++) {
    if ((roomrnum = real_room(jot_converter(frost_pos[i]))) != NOWHERE) {
      if ((mob = read_mobile(jot_converter(85), VIRTUAL)) != NULL) {
        if ((obj = read_object(jot_converter(28), VIRTUAL)) != NULL) {
          obj_to_char(obj, mob);
          perform_wield(mob, obj, TRUE);          
        }
        char_to_room(mob, roomrnum);
      }
    }
  }

  /* Valkyrie */
  if ((roomrnum = real_room(jot_converter(4))) != NOWHERE) {
    if ((mob = read_mobile(jot_converter(82), VIRTUAL)) != NULL) {
      char_to_room(mob, roomrnum);
    }
  }

  /* Remove Brunnhilde */
  for (mob = character_list; mob; mob = mob->next)
    if (GET_MOB_VNUM(mob) == jot_converter(68))
      extract_char(mob);

  PROC_FIRED(ch) = TRUE;
  return 1;
}

SPECIAL(thrym) {
  struct char_data *vict = FIGHTING(ch);
  struct affected_type af;

  if (!ch || cmd || !vict || rand_number(0, 8))
    return 0;

  act("\tCThrym touches you with a chilling hand, freezing you in place.\tn", FALSE, vict, 0, ch, TO_CHAR);
  act("\tCThrym touches $n\tC, freezing $m in place.\tn", FALSE, vict, 0, ch, TO_ROOM);

  new_affect(&af);
  af.spell = SPELL_HOLD_PERSON;
  SET_BIT_AR(af.bitvector, AFF_PARALYZED);
  af.duration = 8;
  affect_join(vict, &af, 1, FALSE, FALSE, FALSE);
  
  return 1;
}

SPECIAL(ymir) {
  if (!ch || cmd)
    return 0;

  if (FIGHTING(ch) && !rand_number(0, 4)) {
    call_magic(ch, FIGHTING(ch), 0, SPELL_FROST_BREATHE, GET_LEVEL(ch), CAST_SPELL);
    return 1;
  }

  return 0;
}

SPECIAL(planetar) {
  if (!ch || cmd)
    return 0;

  if (FIGHTING(ch) && !rand_number(0, 5)) {
    call_magic(ch, FIGHTING(ch), 0, SPELL_LIGHTNING_BREATHE, GET_LEVEL(ch), CAST_SPELL);
    return 1;
  }

  return 0;
}

SPECIAL(gatehouse_guard) {
  struct char_data *mob = (struct char_data *) me;

  if (!IS_MOVE(cmd) || AFF_FLAGGED(mob, AFF_BLIND) || AFF_FLAGGED(mob, AFF_SLEEP) ||
          AFF_FLAGGED(mob, AFF_PARALYZED) || AFF_FLAGGED(mob, AFF_GRAPPLED) ||
          AFF_FLAGGED(mob, AFF_GRAPPLED) || HAS_WAIT(mob))
    return FALSE;

  if (cmd == SCMD_EAST && (!IS_NPC(ch) || IS_PET(ch)) && GET_LEVEL(ch) < 31) {
    act("$N \twblocks your way!\tn\r\n", FALSE, ch, 0, mob, TO_CHAR);
    act("$N \twblocks $n's\tw way!\tn\r\n", FALSE, ch, 0, mob, TO_ROOM);
    return TRUE;
  }

  return 0;
}

/************************************************/
/* end mobile specs, start object specs for jot */
/************************************************/

/* special cloak object proc */
SPECIAL(ymir_cloak) {
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify")) {
    send_to_char(ch, "Invoke ice storm by saying 'icicle storm'.\r\nOnce per day.\r\n");
    return 1;
  }

  struct obj_data *obj = (struct obj_data *) me;

  skip_spaces(&argument);
  if (!is_wearing(ch, 196059)) return 0;
  if (!strcmp(argument, "icicle storm") && cmd_info[cmd].command_pointer == do_say) {
    if (GET_OBJ_SPECTIMER(obj, 0) > 0) {
      send_to_char(ch, "\tcAs you say '\tCicicle storm\tc' to your \tWa cloak of glittering icicles\tc, nothing happens.\tn\r\n");
      return 1;
    }

    weapons_spells("\tBAs you say '\twicicle storm\tB' to $p \tBit flashes bright blue and sends forth a storm of razor sharp icicles in all directions.\tn",
            "\tBAs $n \tBmutters something under his breath  to $p \tBit flashes bright blue and sends forth a storm of razor sharp icicles in all directions.\tn",
            "\tBAs $n \tBmutters something under his breath  to $p \tBit flashes bright blue and sends forth a storm of razor sharp icicles in all directions.\tn",
            ch, 0, (struct obj_data *) me, SPELL_ICE_STORM);

    GET_OBJ_SPECTIMER(obj, 0) = 24;
    return 1;
  }
  return 0;
}
/* mistweave mace object proc */
SPECIAL(mistweave) {
  if (!ch)
    return 0;
  
  if (!cmd && !strcmp(argument, "identify")) {
    send_to_char(ch, "Invoke blindness by saying 'mistweave'. Once per day.\r\n");
    return 1;
  }

  struct obj_data *obj = (struct obj_data *) me;
  struct char_data *vict = FIGHTING(ch);

  if (cmd && argument && CMD_IS("say")) {
    if (!is_wearing(ch, 196012)) 
      return 0;
  
    skip_spaces(&argument);
    
    if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) && 
            !strcmp(argument, "mistweave")) {
      
      if (GET_OBJ_SPECTIMER(obj, 0) > 0) {
        send_to_char(ch, "\tpAs you say '\twmistweave\tp' to your a huge adamantium mace enshrouded with \tWmist\tp, nothing happens.\tn\r\n");
        return 1;
      }
      act("\tLAs you say, '\tnmistweave\tL', "
              "\tLa thick vapor issues forth from $p\tL, "
              "\tLenshrouding the eyes of $N\tL.\tn",
              FALSE, ch, obj, vict, TO_CHAR);
      act("\tLAs $n \tLmutters something under his breath, "
              "\tLa thick vapor issues forth from $p\tL, "
              "\tLenshrouding the eyes of $N.",
              FALSE, ch, obj, vict, TO_ROOM);

      call_magic(ch, FIGHTING(ch), 0, SPELL_BLINDNESS, 30, CAST_SPELL);
      GET_OBJ_SPECTIMER(obj, 0) = 24;
      return 1;
    } else return 0;
  }
  return 0;
}

/* frostbite axe proc */
SPECIAL(frostbite) {
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify")) {
    send_to_char(ch, "Invoke cone of cold  by saying 'frostbite'. Once per day.\r\n");
    return 1;
  }

  struct obj_data *obj = (struct obj_data *) me;
  struct char_data *vict = FIGHTING(ch);
  int pct;
  struct affected_type af;

  if (cmd && argument && CMD_IS("say")) {
    if (!is_wearing(ch, 196000)) 
      return 0;
  
    skip_spaces(&argument);
    
    if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) && 
            !strcmp(argument, "frostbite")) {
      
      if (GET_OBJ_SPECTIMER(obj, 0) > 0) {
        send_to_char(ch, "\tcAs you say '\twfrostbite\tc' to your a \tLa great iron axe \tCrimmed \tLwith \tWfrost\tc, nothing happens.\tn\r\n");
        return 1;
      }
      act("\tCAs you say, '\twfrostbite\tC',\n\r"
              "\tCa swirling gale of pounding ice emanates forth from\n\r"
              "$p \tCpelting your foes.\tn",
              FALSE, ch, obj, 0, TO_CHAR);
      act("\tCAs $n \tCmutters something under his breath,\n\r"
              "\tCa swirling gale of pounding ice emanates forth from\n\r"
              "$p \tCpelting $n's \tCfoes.\tn",
              FALSE, ch, obj, 0, TO_ROOM);

      pct = rand_number(0, 99);
      if (pct < 55)
        call_magic(ch, vict, 0, SPELL_CONE_OF_COLD, 20, CAST_SPELL);
      else if (pct < 85)
        call_magic(ch, vict, 0, SPELL_CONE_OF_COLD, 30, CAST_SPELL);
      else {
        call_magic(ch, vict, 0, SPELL_CONE_OF_COLD, 30, CAST_SPELL);
        new_affect(&af);
        af.spell = SPELL_HOLD_PERSON;
        SET_BIT_AR(af.bitvector, AFF_PARALYZED);
        af.duration = dice(2, 4);
        affect_join(vict, &af, 1, FALSE, FALSE, FALSE);
      }

      GET_OBJ_SPECTIMER(obj, 0) = 24;
      return 1;
    } else return 0;
  }
  return 0;
}

/* special claws gear with proc */
#define VAP_AFFECTS 3
SPECIAL(vaprak_claws) {
  struct affected_type af[VAP_AFFECTS];
  int duration = 0, i = 0;
  
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify")) {
    send_to_char(ch, "Invoke Fury of Vaprak by saying 'vaprak'. Once per day.\r\nWorks only for Trolls and Ogres.\r\n");
    return 1;
  }

  if (GET_RACE(ch) != RACE_OGRE && GET_RACE(ch) != RACE_TROLL)
    return 0;

  struct obj_data *obj = (struct obj_data *) me;

  skip_spaces(&argument);
  if (!is_wearing(ch, 196062)) return 0;
  if (!strcmp(argument, "vaprak") && cmd_info[cmd].command_pointer == do_say) {
    //if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)) {
    if (GET_OBJ_SPECTIMER(obj, 0) > 0) {
      send_to_char(ch, "\trAs you say '\twvaprak\tr' to your claws \tLof the destroyer\tr, nothing happens.\tn\r\n");
      return 1;
    }
    if (affected_by_spell(ch, SKILL_RAGE)) {
      send_to_char(ch, "You are already raging!\r\n");
      return 1;
    }

    weapons_spells("\tLAs you say '\twvaprak\tL' to $p\tL, an evil warmth fills your body.\tn",
            0,
            "\tr$n \trmutters something under his breath.\tn",
            ch, ch, (struct obj_data *) me, 0);

    duration = GET_LEVEL(ch);
    /* init affect array */
    for (i = 0; i < VAP_AFFECTS; i++) {
      new_affect(&(af[i]));
      af[i].spell = SKILL_RAGE;
      af[i].duration = duration;
    }

    af[0].location = APPLY_HITROLL;
    af[0].modifier = 3;

    af[1].location = APPLY_DAMROLL;
    af[1].modifier = 3;

    SET_BIT_AR(af[2].bitvector, AFF_HASTE);
    af[2].location = APPLY_SAVING_WILL;
    af[2].modifier = 3;

    for (i = 0; i < VAP_AFFECTS; i++)
      affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);
    GET_OBJ_SPECTIMER(obj, 0) = 24;
    return 1;
  }
  return 0;
}
#undef VAP_AFFECTS

/* a fake twilight proc (large sword) */
#define TWI_AFFECTS 2

SPECIAL(fake_twilight) {
  struct affected_type af[TWI_AFFECTS];
  struct char_data *vict;
  int duration = 0, i = 0;

  if (!ch)
    return 0;
  
  if (!cmd && !strcmp(argument, "identify")) {
    send_to_char(ch, "Twilight Rage\r\n");
    return 1;
  }
  
  vict = FIGHTING(ch);
        
  if (affected_by_spell(ch, SPELL_BATTLETIDE)) {
    return 0;
  }
  
  if (cmd || !vict || rand_number(0, 18))
    return 0;

  weapons_spells("\tLA glimmer of insanity crosses your face as your\r\n"
          "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
          "\tLA glimmer of insanity crosses $n\tL's face as $s\r\n"
          "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
          "\tLA glimmer of insanity crosses $n\tL's face as $s\r\n"
          "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
          ch, vict, (struct obj_data *) me, 0);

  duration = GET_LEVEL(ch) / 5;
  /* init affect array */
  for (i = 0; i < TWI_AFFECTS; i++) {
    new_affect(&(af[i]));
    af[i].spell = SPELL_BATTLETIDE;
    af[i].duration = duration;
  }

  af[0].location = APPLY_HITROLL;
  af[0].modifier = GET_STR_BONUS(ch);

  af[1].location = APPLY_DAMROLL;
  af[1].modifier = GET_STR_BONUS(ch);

  for (i = 0; i < TWI_AFFECTS; i++)
    affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);

  return 1;
}

/* a twilight proc (large sword) */
SPECIAL(twilight) {
  struct affected_type af[TWI_AFFECTS];
  struct char_data *vict;
  int duration = 0, i = 0;

  if (!ch)
    return 0;
  
  if (!cmd && !strcmp(argument, "identify")) {
    send_to_char(ch, "Twilight Rage\r\n");
    return 1;
  }
  
  vict = FIGHTING(ch);
        
  if (affected_by_spell(ch, SPELL_BATTLETIDE)) {
    return 0;
  }
  
  if (cmd || !vict || rand_number(0, 16))
    return 0;

  weapons_spells("\tLA glimmer of insanity crosses your face as your\r\n"
          "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
          "\tLA glimmer of insanity crosses $n\tL's face as $s\r\n"
          "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
          "\tLA glimmer of insanity crosses $n\tL's face as $s\r\n"
          "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
          ch, vict, (struct obj_data *) me, 0);

  duration = GET_LEVEL(ch) / 5 + 1;
  /* init affect array */
  for (i = 0; i < TWI_AFFECTS; i++) {
    new_affect(&(af[i]));
    af[i].spell = SPELL_BATTLETIDE;
    af[i].duration = duration;
  }

  af[0].location = APPLY_HITROLL;
  af[0].modifier = GET_STR_BONUS(ch);

  af[1].location = APPLY_DAMROLL;
  af[1].modifier = GET_STR_BONUS(ch);

  for (i = 0; i < TWI_AFFECTS; i++)
    affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);

  return 1;
}
#undef TWI_AFFECTS

SPECIAL(valkyrie_sword) {

  if (!ch || cmd)
    return 0;

  if (!cmd && !strcmp(argument, "identify")) {
    send_to_char(ch, "Female Only - Proc Burning Hands\r\n");
    return 1;
  }
  
  if (GET_SEX(ch) != SEX_FEMALE && !IS_NPC(ch)) {
    damage(ch, ch, dice(5, 4), -1, DAM_HOLY, FALSE);
    send_to_char(ch, "\twYou are \tYburned \twby holy light.\tn\r\n");
    act("\tw$n is \tYburned \twby holy light.\tn", FALSE, ch, 0, ch, TO_ROOM);
    return 1;
  }

  struct char_data *vict = FIGHTING(ch);

  if (!is_wearing(ch, 196056) || !vict || rand_number(0, 20))
    return 0;

  weapons_spells(
          "\tYStreaks of flames issue forth from $p\n\r"
          "\tYengulfing your foe.\tn",
          "\tYYou are engulfed by the flames issuing forth from $p.",
          "\tYStreaks of flames issue forth from $p\n\r"
          "\tYengulfing $n's \tYfoe.", ch, vict, (struct obj_data *) me, 0);

  call_magic(ch, vict, 0, SPELL_BURNING_HANDS, 30, CAST_SPELL);

  return 1;
}

SPECIAL(planetar_sword) {
  if (!ch)
    return 0;
  
  if (!cmd && !strcmp(argument, "identify")) {
    send_to_char(ch, "Proc Cure Critic and Dispel Evil\r\n");
    return 1;
  }
  
  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 27))
    return 0;
    
  switch (rand_number(0, 1)) {
    case 1:
      weapons_spells("\tWA nimbus of holy light surrounds your sword, bathing you in its radiance\tn",
              0,
              "\tWA nimbus of holy light surrounds $n's\tW sword, bathing $m in its radiance.", 
              ch, ch, (struct obj_data *) me, SPELL_CURE_CRITIC);
      call_magic(ch, ch, 0, SPELL_CURE_CRITIC, GET_LEVEL(ch), CAST_SPELL);
      return 1;
    case 2:
      weapons_spells("\tWA glowing nimbus of light emanates forth blasting the foul evil in its presence.\tn",
              "\tWA glowing nimbus of light emanates forth from $n, blasting the foul evil in its presence.\tn",
              "\tWA glowing nimbus of light emanates forth from $n, blasting the foul evil in its presence.\tn", 
              ch, vict, (struct obj_data *) me, SPELL_DISPEL_EVIL);
      return 1;
    default:
      return 0;
  }

  return 1;
}

SPECIAL(giantslayer) {
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify")) {
    send_to_char(ch, "Invoke giant hamstring attack by saying 'hamstring'. Once per day.\r\nWorks only for Dwarves.\r\n");
    return 1;
  }

  if (GET_RACE(ch) != RACE_DWARF)
    return 0;

  struct obj_data *obj = (struct obj_data *) me;
  struct char_data *vict = FIGHTING(ch);

  if (!vict)
    return 0;

  skip_spaces(&argument);
  if (!is_wearing(ch, 196066)) return 0;
  if (!strcmp(argument, "hamstring") && cmd_info[cmd].command_pointer == do_say) {
    if (IS_NPC(vict) && GET_RACE(vict) == NPCRACE_GIANT &&
            (vict->in_room == ch->in_room)) {
      if (GET_OBJ_SPECTIMER(obj, 0) > 0) {
        send_to_char(ch, "\tYAs you say '\twhamstring\tY' to your \tLa double-bladed dwarvish axe of \tYgiantslaying, nothing happens.\tn\r\n");
        return 1;
      }

      act("\tyAs you say, '\tLhamstring\ty' to $p\ty,\n\r"
              "\tyit twirls forth from your hand, arcing through the air to "
              "hamstring\n\r$N \tybefore returning to your grasp.\tn",
              FALSE, ch, obj, vict, TO_CHAR);
      act("\tyAs $n \tymutters something under his breath to $p\ty,\n\r"
              "\tyit twirls forth from $s hand, arcing through the air to "
              "hamstring\n\r$N \tybefore returning to your grasp.\tn",
              FALSE, ch, obj, vict, TO_ROOM);
      // We hamstring the foe
      act("$N falls to $S knees before you!",
              FALSE, ch, obj, vict, TO_CHAR);
      act("$N falls to $S knees before $n!",
              FALSE, ch, obj, vict, TO_NOTVICT);
      act("You fall to your knees in agony!",
              FALSE, ch, obj, vict, TO_VICT);
      WAIT_STATE(vict, PULSE_VIOLENCE * 2);
      GET_POS(vict) = POS_SITTING;
      GET_HIT(vict) -= 100;

      GET_OBJ_SPECTIMER(obj, 0) = 24;
      return 1; // end for
    } else {
      send_to_char(ch, "\tYAs you say '\twhamstring\tY' to your \tLa double-bladed dwarvish axe of \tYgiantslaying, nothing happens.\tn\r\n");
      return 1;
    }
    return 0;
  }
  return 0;
}

#undef JOT_VNUM 
#undef MAX_FG   // fire giants
#undef MAX_SB   // smoking beard batallion
#undef MAX_EM  // efreeti mercenaries
#undef MAX_FROST   // frost giants

/*****************/
/* End Jot       */
/*****************/



/* put new zone procs here */
