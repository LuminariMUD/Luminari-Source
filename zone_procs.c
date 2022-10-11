/**************************************************************************
 *  File: zone_procs.c                                 Part of LuminariMUD *
 *  Usage: Special procedures for zones                                    *
 *  Author:  Zusuk                                                         *
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
#include "act.h"        /* for act related stuff, like act.offensive fuctions */
#include "spec_procs.h" /**< zone_procs.c is part of the spec_procs module */
#include "fight.h"
#include "graph.h"
#include "mud_event.h"
#include "actions.h"
#include "domains_schools.h"
#include "spec_abilities.h"
#include "treasure.h"
#include "mobact.h"       /* for npc_find_target() */
#include "dg_scripts.h"   /* for load_mtrigger() */
#include "staff_events.h" /* for staff events!  prisoner treasury! */

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
static void castle_mob_spec(mob_vnum mobnum, SPECIAL_DECL(*specproc));

/* end head of file */

/******************************************************************/
/*  KINGS CASTLE */
/******************************************************************/

/* Special procedures for Kings Castle by Pjotr. Coded by Sapowox. */
SPECIAL_DECL(CastleGuard);
SPECIAL_DECL(James);
SPECIAL_DECL(cleaning);
SPECIAL_DECL(DicknDavid);
SPECIAL_DECL(tim);
SPECIAL_DECL(tom);
SPECIAL_DECL(king_welmar);
SPECIAL_DECL(training_master);
SPECIAL_DECL(peter);
SPECIAL_DECL(jerry);

/* IMPORTANT! The below defined number is the zone number of the Kings Castle.
 * Change it to apply to your chosen zone number.
 * */
#define Z_KINGS_C 150

/* Assign castle special procedures. NOTE: The mobile number isn't fully
 * specified. It's only an offset from the zone's base. */
static void castle_mob_spec(mob_vnum mobnum, SPECIAL_DECL(*specproc))
{
  mob_vnum vmv = castle_virtual(mobnum);
  mob_rnum rmr = NOBODY;

  if (vmv != NOBODY)
    rmr = real_mobile(vmv);

  if (rmr == NOBODY)
  {
    if (!mini_mud)
      log("SYSERR: assign_kings_castle(): can't find mob #%d.", vmv);
    /* SYSERR_DESC: When the castle_mob_spec() function is given a mobnum
     * that does not correspond to a mod loaded (when not in minimud mode),
     * this error will result. */
  }
  else
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
  castle_mob_spec(0, CastleGuard);      /* Gwydion */
  castle_mob_spec(1, king_welmar);      /* Our dear friend, the King */
  castle_mob_spec(3, CastleGuard);      /* Jim */
  castle_mob_spec(4, CastleGuard);      /* Brian */
  castle_mob_spec(5, CastleGuard);      /* Mick */
  castle_mob_spec(6, CastleGuard);      /* Matt */
  castle_mob_spec(7, CastleGuard);      /* Jochem */
  castle_mob_spec(8, CastleGuard);      /* Anne */
  castle_mob_spec(9, CastleGuard);      /* Andrew */
  castle_mob_spec(10, CastleGuard);     /* Bertram */
  castle_mob_spec(11, CastleGuard);     /* Jeanette */
  castle_mob_spec(12, peter);           /* Peter */
  castle_mob_spec(13, training_master); /* The training master */
  castle_mob_spec(16, James);           /* James the Butler */
  castle_mob_spec(17, cleaning);        /* Ze Cleaning Fomen */
  castle_mob_spec(20, tim);             /* Tim, Tom's twin */
  castle_mob_spec(21, tom);             /* Tom, Tim's twin */
  castle_mob_spec(24, DicknDavid);      /* Dick, guard of the Treasury */
  castle_mob_spec(25, DicknDavid);      /* David, Dicks brother */
  castle_mob_spec(26, jerry);           /* Jerry, the Gambler */
  castle_mob_spec(27, CastleGuard);     /* Michael */
  castle_mob_spec(28, CastleGuard);     /* Hans */
  castle_mob_spec(29, CastleGuard);     /* Boris */
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

  iVictim = rand_number(0, iNum_bad_guys); /* How nice, we give them a chance */
  if (!iVictim)
    return (NULL);

  iNum_bad_guys = 0;

  for (ch = world[IN_ROOM(chAtChar)].people; ch; ch = ch->next_in_room)
  {
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

/* Do_npc_rescue. Makes ch_hero rescue ch_victim. Used by Tim and Tom.
   not to be mistaken for npc_rescue() for AI in mobact.c */
int do_npc_rescue(struct char_data *ch_hero, struct char_data *ch_victim)
{
  struct char_data *ch_bad_guy;

  for (ch_bad_guy = world[IN_ROOM(ch_hero)].people;
       ch_bad_guy && (FIGHTING(ch_bad_guy) != ch_victim);
       ch_bad_guy = ch_bad_guy->next_in_room)
    ;

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

  if (ch->points.psp < 10)
    return;

  /* Find someone suitable to fry ! */
  if (!(tch = get_victim(ch)))
    return;

  switch (rand_number(0, 8))
  {
  case 1:
  case 2:
  case 3:
    send_to_char(ch, "You raise your hand in a dramatical gesture.\r\n");
    act("$n raises $s hand in a dramatical gesture.", 1, ch, 0, 0, TO_ROOM);
    cast_spell(ch, tch, 0, SPELL_COLOR_SPRAY, 0);
    break;
  case 4:
  case 5:
    send_to_char(ch, "You concentrate and mumble to yourself.\r\n");
    act("$n concentrates, and mumbles to $mself.", 1, ch, 0, 0, TO_ROOM);
    cast_spell(ch, tch, 0, SPELL_HARM, 0);
    break;
  case 6:
  case 7:
    act("You look deeply into the eyes of $N.", 1, ch, 0, tch, TO_CHAR);
    act("$n looks deeply into the eyes of $N.", 1, ch, 0, tch, TO_NOTVICT);
    act("You see an ill-boding flame in the eye of $n.", 1, ch, 0, tch, TO_VICT);
    cast_spell(ch, tch, 0, SPELL_FIREBALL, 0);
    break;
  default:
    if (!rand_number(0, 1))
      cast_spell(ch, ch, 0, SPELL_HEAL, 0);
    break;
  }

  ch->points.psp -= 10;

  return;
}

/* King_welmar. Control the actions and movements of the King. */
SPECIAL(king_welmar)
{
  char actbuf[MAX_INPUT_LENGTH] = {'\0'};

  const char *monolog[] = {
      "$n proclaims 'Primus in regnis Geticis coronam'.",
      "$n proclaims 'regiam gessi, subiique regis'.",
      "$n proclaims 'munus et mores colui sereno'.",
      "$n proclaims 'principe dignos'."};

  const char bedroom_path[] = "s33004o1c1S.";
  const char throne_path[] = "W3o3cG52211rg.";
  const char monolog_path[] = "ABCDPPPP.";

  static const char *path = NULL;
  static int path_index = 0;
  static bool move = FALSE;

  if (!move)
  {
    if (time_info.hours == 8 && IN_ROOM(ch) == castle_real_room(51))
    {
      move = TRUE;
      path = throne_path;
      path_index = 0;
    }
    else if (time_info.hours == 21 && IN_ROOM(ch) == castle_real_room(17))
    {
      move = TRUE;
      path = bedroom_path;
      path_index = 0;
    }
    else if (time_info.hours == 12 && IN_ROOM(ch) == castle_real_room(17))
    {
      move = TRUE;
      path = monolog_path;
      path_index = 0;
    }
  }
  if (cmd || (GET_POS(ch) < POS_SLEEPING) ||
      (GET_POS(ch) == POS_SLEEPING && !move))
    return (FALSE);

  if (FIGHTING(ch))
  {
    fry_victim(ch);
    return (FALSE);
  }
  else if (banzaii(ch))
    return (FALSE);

  if (!move)
    return (FALSE);

  switch (path[path_index])
  {
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
    change_position(ch, POS_STANDING);
    act("$n awakens and stands up.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'S':
    change_position(ch, POS_SLEEPING);
    act("$n lies down on $s beautiful bed and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'r':
    change_position(ch, POS_SITTING);
    act("$n sits down on $s great throne.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 's':
    change_position(ch, POS_STANDING);
    act("$n stands up.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'G':
    act("$n says 'Good morning, trusted friends.'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'g':
    act("$n says 'Good morning, dear subjects.'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'o':
    do_gen_door(ch, strcpy(actbuf, "door"), 0, SCMD_UNLOCK); /* strcpy: OK */
    do_gen_door(ch, strcpy(actbuf, "door"), 0, SCMD_OPEN);   /* strcpy: OK */
    break;

  case 'c':
    do_gen_door(ch, strcpy(actbuf, "door"), 0, SCMD_CLOSE); /* strcpy: OK */
    do_gen_door(ch, strcpy(actbuf, "door"), 0, SCMD_LOCK);  /* strcpy: OK */
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

  if (rand_number(0, 1))
  {
    tch = pupil1;
    pupil1 = pupil2;
    pupil2 = tch;
  }

  switch (rand_number(0, 7))
  {
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

  if ((king = find_npc_by_name(ch, "King Welmar", 11)) != NULL)
  {
    char actbuf[MAX_INPUT_LENGTH];

    if (!ch->master)
      do_follow(ch, strcpy(actbuf, "King Welmar"), 0, 0); /* strcpy: OK */
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

  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
  {
    if (!is_trash(i))
      continue;

    if (gripe)
    {
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
    switch (rand_number(0, 5))
    {
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

  if (rand_number(0, 1))
  {
    tch = gambler1;
    gambler1 = gambler2;
    gambler2 = tch;
  }

  switch (rand_number(0, 5))
  {
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

#define ZONE_VNUM 1423

/* just made this to help facilitate switching of zone vnums if needed */
int calc_room_num(int value)
{
  return (ZONE_VNUM * 100) + value;
}

/* this proc swaps exits in the rooms in a given area */
SPECIAL(abyss_randomizer)
{
  struct char_data *i = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  if (cmd)
    return 0;

  if (rand_number(0, 9))
    return 0;

  int room, temp1, temp2;

  for (room = calc_room_num(1); room <= calc_room_num(18); room++)
  {

    /* Swapping North and South */
    if (world[real_room(room)].dir_option[NORTH] && world[real_room(room)].dir_option[NORTH]->to_room != NOWHERE)
      temp1 = world[real_room(room)].dir_option[NORTH]->to_room;
    else
      temp1 = NOWHERE;
    if (world[real_room(room)].dir_option[SOUTH] && world[real_room(room)].dir_option[SOUTH]->to_room != NOWHERE)
      temp2 = world[real_room(room)].dir_option[SOUTH]->to_room;
    else
      temp2 = NOWHERE;
    if (temp2 != NOWHERE)
    {
      if (!world[real_room(room)].dir_option[NORTH])
        CREATE(world[real_room(room)].dir_option[NORTH], struct room_direction_data, 1);
      world[real_room(room)].dir_option[NORTH]->to_room = temp2;
    }
    else if (world[real_room(room)].dir_option[NORTH])
    {
      free(world[real_room(room)].dir_option[NORTH]);
      world[real_room(room)].dir_option[NORTH] = NULL;
    }
    if (temp1 != NOWHERE)
    {
      if (!world[real_room(room)].dir_option[SOUTH])
        CREATE(world[real_room(room)].dir_option[SOUTH], struct room_direction_data, 1);
      world[real_room(room)].dir_option[SOUTH]->to_room = temp1;
    }
    else if (world[real_room(room)].dir_option[SOUTH])
    {
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
    if (temp2 != NOWHERE)
    {
      if (!world[real_room(room)].dir_option[EAST])
        CREATE(world[real_room(room)].dir_option[EAST], struct room_direction_data, 1);
      world[real_room(room)].dir_option[EAST]->to_room = temp2;
    }
    else if (world[real_room(room)].dir_option[EAST])
    {
      free(world[real_room(room)].dir_option[EAST]);
      world[real_room(room)].dir_option[EAST] = NULL;
    }
    if (temp1 != NOWHERE)
    {
      if (!world[real_room(room)].dir_option[WEST])
        CREATE(world[real_room(room)].dir_option[WEST], struct room_direction_data, 1);
      world[real_room(room)].dir_option[WEST]->to_room = temp1;
    }
    else if (world[real_room(room)].dir_option[WEST])
    {
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
    if (temp2 != NOWHERE)
    {
      if (!world[real_room(room)].dir_option[UP])
        CREATE(world[real_room(room)].dir_option[UP], struct room_direction_data, 1);
      world[real_room(room)].dir_option[UP]->to_room = temp2;
    }
    else if (world[real_room(room)].dir_option[UP])
    {
      free(world[real_room(room)].dir_option[UP]);
      world[real_room(room)].dir_option[UP] = NULL;
    }
    if (temp1 != NOWHERE)
    {
      if (!world[real_room(room)].dir_option[DOWN])
        CREATE(world[real_room(room)].dir_option[DOWN], struct room_direction_data, 1);
      world[real_room(room)].dir_option[DOWN]->to_room = temp1;
    }
    else if (world[real_room(room)].dir_option[DOWN])
    {
      free(world[real_room(room)].dir_option[DOWN]);
      world[real_room(room)].dir_option[DOWN] = NULL;
    }
  }

  snprintf(buf, sizeof(buf), "\tpThe world seems to shift.\tn\r\n");

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

#define CF_VNUM 1060

/* just made this to help facilitate switching of zone vnums if needed */
int cf_converter(int value)
{
  return (CF_VNUM * 100) + value;
}

/* this proc will cause the training master to sick all his minions to track
   whoever he is fighting - will fire one time and that's it */
SPECIAL(cf_trainingmaster)
{
  struct char_data *i = NULL;
  struct char_data *enemy = NULL;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return 0;

  enemy = FIGHTING(ch);

  if (!enemy)
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
  {
    if (enemy->master && enemy->master->in_room == enemy->in_room)
      enemy = enemy->master;
    act("$n waves $s hand slightly.", FALSE, ch, 0, 0, TO_ROOM);
    for (i = character_list; i; i = i->next)
    {
      if (!FIGHTING(i) && IS_NPC(i) && (GET_MOB_VNUM(i) == cf_converter(32) || GET_MOB_VNUM(i) == cf_converter(33) || GET_MOB_VNUM(i) == cf_converter(34) || GET_MOB_VNUM(i) == cf_converter(35) || GET_MOB_VNUM(i) == cf_converter(36) || GET_MOB_VNUM(i) == cf_converter(37) || GET_MOB_VNUM(i) == cf_converter(38) || GET_MOB_VNUM(i) == cf_converter(39)) && ch != i)
      {
        if (ch->in_room != i->in_room)
        {
          HUNTING(i) = enemy;
          hunt_victim(i);
        }
        else
          hit(ch, enemy, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }
    } // for loop

    PROC_FIRED(ch) = TRUE;
    return 1;
  }

  return 0;
}

/* this is lord alathar's proc to summon his bodyguards to him */
SPECIAL(cf_alathar)
{
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

  for (i = 50; i < 57; i++)
  {
    mob = read_mobile(cf_converter(i), VIRTUAL);
    if (mob)
    {
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
/* The Prisoner  */
/*****************/

/* objects */

/* unfinished */
SPECIAL(tia_rapier)
{
  struct char_data *vict = NULL;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: dancing parry - on parry will do a light vamp attack\r\n");
    send_to_char(ch, "Proc: dragon strike - 120 to 200 energy damage\r\n");
    send_to_char(ch, "Proc: dragon gaze - paralyze opponent\r\n");
    return TRUE;
  }

  if (!ch || cmd || GET_POS(ch) == POS_DEAD)
    return 0;

  vict = FIGHTING(ch);

  if (!vict)
    return 0;

  if (!strcmp(argument, "parry"))
  {
    act("\tLYour \tcrapier \tCglows brightly\tL as it steals some \trlifeforce\tn "
        "\tLfrom $N \tLand transfers it back to you.\tn",
        FALSE, ch, (struct obj_data *)me, vict, TO_CHAR);
    act("$n's \tcrapier \tCglows brightly\tL as it steals some \trlifeforce\tn "
        "\tLfrom $N\tL.\tn",
        FALSE, ch, (struct obj_data *)me, vict, TO_NOTVICT);
    act("$n's \tcrapier \tCglows brightly\tL as it steals some \trlifeforce\tn "
        "\tLfrom you and transfers it back to $m.\tn",
        FALSE, ch, (struct obj_data *)me, vict, TO_VICT);
    damage(ch, vict, dice(5, 5), -1, DAM_ENERGY, FALSE); // type -1 = no dam message
    process_healing(vict, ch, -1, (dice(5, 5) + GET_DEX_BONUS(ch)), 0);
    return 1;
  }

  if (vict)
  {
    if (!rand_number(0, 20))
    {
      act("\tWA \tBwave \tWof \tDdarkness \tBoozes \tWslowly from your sword, \tbengulfing \tWthe \tn\r\n"
          "\tWarea in a \tLvoid \tWof \tLblack.\tW  You begin to perceive the \tBfaint outline \tn\r\n"
          "\tWof a \tBdragon\tW surrouding your \tbrapier. \tWThe \tBimage \tWbegins to fiercely \tbclaw \tn\r\n"
          "\tWand \tBsavagely \tbbite \tWat \tn$N's \tWbody.\tn",
          FALSE, ch, 0, vict, TO_CHAR);
      act("\twA \tBwave\tW of \tLdarkness \tBoozes \tWslowly from \tb$n's \tWsword, \tbengulfing \tWthe \tn\r\n"
          "\tWarea in a \tLvoid \tWof \tLblack.\tW  You begin to perceive the \tBfaint outline \tn\r\n"
          "\tWof a \tBdragon\tW surrouding \tB&s \tbrapier.\tW  The \tBimage \tWbegins to fiercely \tbclaw \tn\r\n"
          "\tWand \tBsavagely \tbbite \tWat \tn$N's \tWbody.\tn",
          FALSE, ch, 0, vict, TO_ROOM);
      damage(ch, vict, rand_number(120, 200), -1, DAM_ENERGY, FALSE); // type -1 = no dam message
      return 1;
    }

    if (!rand_number(0, 50))
    {
      weapons_spells("\tWSuddenly your \tn$p\tW is enveloped by \tbsheer \tLdarkness, \tWleaving only a pair of \tn\r\n"
                     "\tBblazing eyes \tWgazing directly into the \tBsoul\tW of \tn$N\tW.  A sudden wave of \tBterror \tbovercomes \tn\r\n"
                     "\tn$N\tW, who begins to \tbtremble violently\tW and lose \tBcontrol \tWof $s senses.\tn",

                     "\tW$n's \tWsword is enveloped by \tbsheer \tLdarkness,\tW leaving only a pair of \tBblazing eyes\tW gazing \tn\r\n"
                     "\tWdirectly into the \tBsoul\tW of \tn$N\tW.  A sudden wave of \tBterror \tbovercomes \tn$n\tW, who begins to \tn\r\n"
                     "\tbtremble violenty \tWand lose \tBcontrol\tW of $s senses.\tn",

                     "\tW$n's sword is enveloped by \tbsheer \tLdarkness,\tW leaving only a pair of \tBblazing eyes\tW gazing \tn\r\n"
                     "\tWdirectly into the \tBsoul\tb of \tn$N\tW.  A sudden wave of \tBterror \tbovercomes \tn$N\tW, who begins\tn\r\n"
                     "\tWto \tbtremble violenty \tWand lose \tBcontrol \tWof $S senses.\tn",
                     ch, vict, (struct obj_data *)me, SPELL_IRRESISTIBLE_DANCE);
      return 1;
    }
  }

  return 0;
}

/*****************************************************************/
/* mobiles */
/*****************************************************************/

/* globals */

/* the prisoner battle */
/* prisoner_heads:
     -1 represents that the prisoner hasn't been killed yet
     -2 represents that the prisoner has been fully killed */
int prisoner_heads = -1;
bool eq_loaded = FALSE;

/* end globals */

int check_heads(struct char_data *ch)
{

  /* green head dies */
  if (prisoner_heads == 5)
  {
    act("\tLYour blood \tWfreezes\tL as the \tggreen \tLhead of the Prisoner screams\n\r"
        "\tLa horrifying wail of pain and drops to the floor, out of the battle!\tn",
        FALSE, ch, 0, 0, TO_ROOM);
    act("\n\r\tLThe remaining four heads turn and gaze at you with a glare of hatred.\tn",
        FALSE, ch, 0, 0, TO_ROOM);
    prisoner_heads = 4;
    return 1;
  }

  /* white head dies */
  if (prisoner_heads == 4)
  {
    act("\tLYour blood \tWfreezes\tL as the \tWwhite \tLhead of the Prisoner screams\n\r"
        "\tLa horrifying wail of pain and drops to the floor, out of the battle!\tn",
        FALSE, ch, 0, 0, TO_ROOM);
    act("\n\r\tLThe remaining three heads turn and gaze at you with a glare of hatred.\tn",
        FALSE, ch, 0, 0, TO_ROOM);
    prisoner_heads = 3;
    return 1;
  }

  /* black head dies */
  if (prisoner_heads == 3)
  {
    act("\tLYour blood \tWfreezes\tL as the black head of the Prisoner screams\n\r"
        "\tLa horrifying wail of pain and drops to the floor, out of the battle!\tn",
        FALSE, ch, 0, 0, TO_ROOM);
    act("\n\r\tLThe remaining two heads turn and gaze at you with a glare of hatred.\tn",
        FALSE, ch, 0, 0, TO_ROOM);
    prisoner_heads = 2;
    return 1;
  }

  /* blue head dies */
  if (prisoner_heads == 2)
  {
    act("\tLYour blood \tWfreezes\tL as the \tBblue \tLhead of the Prisoner screams\n\r"
        "\tLa horrifying wail of pain and drops to the floor, out of the battle!\tn",
        FALSE, ch, 0, 0, TO_ROOM);
    act("\n\r\tLThe remaining \trred \tLhead turns and gazes at you with a glare of hatred.\tn",
        FALSE, ch, 0, 0, TO_ROOM);
    prisoner_heads = 1;
    return 1;
  }

  /* exit */
  return 0;
}

/* gotta have this here, incase we gotta reload the Prisoner at death cause of heads still remaining*/
void move_items(struct char_data *ch, struct char_data *lich)
{
  struct obj_data *item;
  struct obj_data *next_item;
  int pos;
  for (item = ch->carrying; item; item = next_item)
  {
    next_item = item->next_content;
    obj_from_char(item);
    obj_to_char(item, lich); /* transfer any eq and inv */
  }
  for (pos = 0; pos < NUM_WEARS; pos++)
  {
    if (ch->equipment[pos] != NULL)
    {
      item = unequip_char(ch, pos);
      equip_char(lich, item, pos);
    }
  }
}

#define THE_PRISONER 113750
#define DRACOLICH_PRISONER 113751
void prisoner_on_death(struct char_data *ch)
{
  struct char_data *prisoner = NULL;
  struct char_data *tch = NULL;
  struct affected_type af;

  /*Still got HEADS!!, means they did lots of damage etc..*/
  if (prisoner_heads > 1)
  {
    check_heads(ch); // to get right message..

    prisoner = read_mobile(THE_PRISONER, VIRTUAL);
    char_to_room(prisoner, ch->in_room);
    change_position(prisoner, POS_STANDING);

    move_items(ch, prisoner);

    IS_CARRYING_W(prisoner) = 0;
    IS_CARRYING_N(prisoner) = 0;
    load_mtrigger(prisoner);
    /* the fight.c death code should remove the old prisoner */

    return;
  }

  /******* dracolich transition! *********************/
  else
  {
    /* red head dies, the last head - we are really loading the dracolich here */
    prisoner = read_mobile(DRACOLICH_PRISONER, VIRTUAL);
    char_to_room(prisoner, ch->in_room);
    change_position(prisoner, POS_STANDING);

    move_items(ch, prisoner);

    IS_CARRYING_W(prisoner) = 0;
    IS_CARRYING_N(prisoner) = 0;
    load_mtrigger(prisoner);
    /* the fight.c death code should remove the old prisoner */
  }

  /* we are transitioning to dracolich!!, ch is no longer relevant hopefully */

  /* the player experience for the transition follows! */
  act("\tLWith a horrifying sound like a fearsome roar mixed with the screams of\n\r"
      "\tLexcruciating pain, the mighty Prisoner calls on her remaining divine power.\n\r"
      "\tWBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM!\n\r\n\r\n\r\n\r"
      "\tLA blinding light \tf\tWFLASHES\tn\tL from within her massive body followed by an\n\r"
      "\tLexplosion so forceful and loud that your ears begin to \trbleed even before\n\r"
      "\tryour body is hurled with tremendous force against the rumbling cavern walls",
      FALSE, prisoner, 0, 0, TO_ROOM);

  for (tch = world[prisoner->in_room].people; tch; tch = tch->next_in_room)
  {
    if (tch != prisoner && tch != ch && GET_LEVEL(tch) < LVL_IMMORT)
    {
      if (GET_POS(tch) > POS_SITTING)
        change_position(tch, POS_SITTING);
      WAIT_STATE(tch, PULSE_VIOLENCE * 3);
    }
  }

  WAIT_STATE(prisoner, PULSE_VIOLENCE * 2);

  act("\trThrough a haze of dizziness you look up..\tn\r\n"
      "\tr.\tn\r\n\tr.\tn\r\n\trA most horrifying transformation takes place before you.\tn\r\n  "
      "\trThe flesh catches fire on her dead body, burning quickly away to blackened bone.\n\r\n  "
      "\tLThe bones \tYglow\tL with magic, it's eyes flare \tRRed\tL as the entire skeleton \n\r\n"
      "\tLrises from the ashes of death. With a sinister gaze, the new-born \n\r\n"
      "\tLDracoLich of the Prisoner utters arcane words of power, and turns to face you... \n\r\n"
      "\tWYou freeze in terror at the sight of the thing, momentarily frozen until the \n\r\n"
      "\tWrealization of this extreme danger sinks in. You fight back the dizziness.\n\r\n",
      FALSE, prisoner, 0, 0, TO_ROOM);
  act("\tLSuddenly everything fades to black...\tn",
      FALSE, prisoner, 0, 0, TO_ROOM);

  for (tch = world[prisoner->in_room].people; tch; tch = tch->next_in_room)
  {
    if (tch && tch != prisoner && tch != ch && GET_LEVEL(tch) < LVL_IMMORT)
    {
      WAIT_STATE(tch, PULSE_VIOLENCE * 3);

      new_affect(&af);
      af.spell = SPELL_SLEEP;
      af.duration = 5;
      SET_BIT_AR(af.bitvector, AFF_SLEEP);
      affect_join(tch, &af, FALSE, FALSE, TRUE, FALSE);
      if (GET_POS(tch) >= POS_SLEEPING)
        change_position(tch, POS_STUNNED);

      damage(prisoner, tch, rand_number(600, 900), TYPE_UNDEFINED, DAM_MENTAL, FALSE);
    }
  }

  return;
}

int rejuv_prisoner(struct char_data *ch)
{
  int rejuv = 0;

  if (!rand_number(0, 7) && GET_HIT(ch) < GET_MAX_HIT(ch) && PROC_FIRED(ch) == FALSE && !FIGHTING(ch))
  {
    rejuv = GET_HIT(ch) + 1500;

    if (rejuv >= GET_MAX_HIT(ch))
      rejuv = GET_MAX_HIT(ch);

    GET_HIT(ch) = rejuv;

    PROC_FIRED(ch) = TRUE;

    act("\trThe blood-red wounds on the Prisoner's body begin to close as she is partially revived!\tn",
        FALSE, ch, 0, 0, TO_ROOM);

    return 1;
  }
  else if (!rand_number(0, 4))
  {
    PROC_FIRED(ch) = FALSE;
  }

  if (!rand_number(0, 10) && FIGHTING(ch) && GET_HIT(ch) < GET_MAX_HIT(ch))
  {
    rejuv = GET_HIT(ch) + 2500;

    if (rejuv >= GET_MAX_HIT(ch))
      rejuv = GET_MAX_HIT(ch);

    GET_HIT(ch) = rejuv;

    act("\tLThe Prisoner ROARS in anger, and throws her talons to the sky furiously!\r\n"
        "\tWWhite tendrils of power crackle through the air, flowing into the Prisoner!",
        FALSE, ch, 0, 0, TO_ROOM);
    act("\trThe blood-red wounds on the Prisoner's body begin to close as she is partially revived!\tn",
        FALSE, ch, 0, 0, TO_ROOM);

    return 1;
  }

  return 0;
}

int prisoner_attacks(struct char_data *ch)
{
  if (!ch)
    return 0;

  if (!FIGHTING(ch))
    return 0;

  if (!rand_number(0, 2))
  {
    int breaths = 0;
    int breath[5];
    int selected = 0;

    if (prisoner_heads >= 1)
      breath[breaths++] = SPELL_FIRE_BREATHE;
    if (prisoner_heads >= 2)
      breath[breaths++] = SPELL_LIGHTNING_BREATHE;
    if (prisoner_heads >= 3)
      breath[breaths++] = SPELL_ACID_BREATHE;
    if (prisoner_heads >= 4)
      breath[breaths++] = SPELL_FROST_BREATHE;
    if (prisoner_heads >= 5)
      breath[breaths++] = SPELL_GAS_BREATHE;

    if (breaths < 1)
      return 0;

    selected = dice(1, breaths) - 1;
    selected = breath[selected];

    // do a breath..  level 40 breath since she breaths every round.
    call_magic(ch, FIGHTING(ch), 0, selected, 0, 34, CAST_INNATE);

    return 1;
  }
  else if (!rand_number(0, 2) && perform_tailsweep(ch))
  {
    /* looks like we did the tailsweeep successffully to at least one victim */
    return 1;
  }
  else if (!rand_number(0, 2) && perform_dragonbite(ch, FIGHTING(ch)))
  {
    /* looks like we did the dragonbite to at least one victim */
    return 1;
  }
  else if (!rand_number(0, 2))
  {
    int i = 0;

    /* spam some attacks */
    for (i = 0; i <= rand_number(4, 8); i++)
    {
      if (valid_fight_cond(ch, TRUE))
        hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    }
    return 1;
  }

  return 0;
}

/*************************************/
/****** prisoner gear defines! *******/
/*************************************/
/* unique */
#define MALEVOLENCE 132101      /* 00 */
#define CELESTIAL_SWRD 132300   /* 01 */
#define HELL_SWRD 132302        /* 02 */
#define MAGI_STAFF 132109       /* 03 */
#define MOONBLADE 132118        /* 04 */
#define DROW_SCIMITAR 132126    /* 05 */
#define CRYSTAL_RAPIER 132125   /* 06 */
#define STAR_CRICLET 132104     /* 07 */
#define HOLY_PLATE 132105       /* 08 */
#define DRAGONBONE_PLATE 132116 /* 09 */
#define SPEED_GAUNT 132128      /* 10 */
#define SHADOW_CLOAK 132120     /* 11 */
#define ELVEN_CLOAK 132106      /* 12 */
#define RUNED_QUIVER 132119     /* 13 */
#define SLAADI_GOGS 132121      /* 14 */
#define MANDRAKE_EAR 132117     /* 15 */
#define MITH_ARROW 132127       /* 16 */
#define ARM_VALOR 132103        /* 17 */
#define BLACK_FIGURINE 132114   /* 18 */
#define STABILITY_BOOTS 132133  /* 19 */
#define TOP_UNIQUES 19
/* base items */
#define WEAPON_OIL 132131
#define WEAPON_POISON 132132
#define LAVANDER_VIA 132110
#define COINS_GOLD 132112
#define COINS_PLAT 132111
#define COINS_SILV 132113

/*************************************/
/* variables */
#define PRISONER_VAULT 132100
#define VALID_VNUM_LOW 132100
#define VALID_VNUM_HiGH 132399
#define TOP_UNIQUES_OIL 21
#define NUM_TREASURE 7
#define LOOP_LIMIT 1000
/*************************************/
/*************************************/

/* this function is meant to load the gear into the treasury
   we are checking 1) the items haven't loaded and 2) that the prisoner's final form is engaged in combat to trigger this section */
void prisoner_gear_loading(struct char_data *ch)
{
  struct obj_data *olist = NULL;
  // struct obj_data *tobj = NULL;
  bool loaded = FALSE;
  int ovnum = NOTHING, loop_counter = 0, num_items = 0, num_treasure = 0;

  int objNums[TOP_UNIQUES + 1] = {
      MALEVOLENCE,      /* for warrior, berserker, giantslayer, battlerager */
      CELESTIAL_SWRD,   /* good only warrior types? - index 1 */
      HELL_SWRD,        /* evil only warrior types? */
      MAGI_STAFF,       /* wizard types */
      MOONBLADE,        /* bladesinger, ranger */
      DROW_SCIMITAR,    /* shadowstalker, weaponmaster - index 5 */
      CRYSTAL_RAPIER,   /* swashbuckler */
      STAR_CRICLET,     /* caster circlet */
      HOLY_PLATE,       /* good only arnmor ? */
      DRAGONBONE_PLATE, /* heavy armor ? */
      SPEED_GAUNT,      /* monk gauntlets - index 10 */
      SHADOW_CLOAK,     /* evil rogue cloak? */
      ELVEN_CLOAK,      /* good elven rogue cloak? */
      RUNED_QUIVER,     /* quiver */
      SLAADI_GOGS,      /* psi eyewear */
      MANDRAKE_EAR,     /* earring - index 15 */
      MITH_ARROW,       /* arrows */
      ARM_VALOR,        /* armplates of valor */
      BLACK_FIGURINE,   /* summons gargoyle */
      STABILITY_BOOTS,  /* stability boots! - index 19 */
  };

  /* we are giving a random weapon oil, lets have a list of options! */
  int wpnOils[TOP_UNIQUES_OIL + 1] = {
      WEAPON_SPECAB_SEEKING, /* 0 */
      WEAPON_SPECAB_ADAPTIVE,
      WEAPON_SPECAB_DISRUPTION,
      WEAPON_SPECAB_DEFENDING,
      WEAPON_SPECAB_EXHAUSTING,
      WEAPON_SPECAB_CORROSIVE, /* 5 */
      WEAPON_SPECAB_SPEED,
      WEAPON_SPECAB_GHOST_TOUCH,
      WEAPON_SPECAB_BLINDING,
      WEAPON_SPECAB_SHOCK,
      WEAPON_SPECAB_FLAMING, /* 10 */
      WEAPON_SPECAB_THUNDERING,
      WEAPON_SPECAB_AGILE,
      WEAPON_SPECAB_WOUNDING,
      WEAPON_SPECAB_LUCKY,
      WEAPON_SPECAB_BEWILDERING, /* 15 */
      WEAPON_SPECAB_KEEN,
      WEAPON_SPECAB_VICIOUS,
      WEAPON_SPECAB_INVIGORATING,
      WEAPON_SPECAB_VORPAL,
      WEAPON_SPECAB_VAMPIRIC, /* 20 */
      WEAPON_SPECAB_BANE,     /* 21 */
  };

  if (!ch)
    return;

  if (IS_STAFF_EVENT && STAFF_EVENT_NUM == THE_PRISONER_EVENT)
    num_treasure = rand_number(NUM_TREASURE + 1, TOP_UNIQUES - 2);
  else
    num_treasure = NUM_TREASURE;

  /* this loop will only run once, it gets turned off by a variable below */
  do
  {
    /* pick an item, any item! */
    ovnum = objNums[rand_number(0, TOP_UNIQUES)];

    /* make sure this item isn't a duplicate */
    /* loop through vault items */
    for (olist = world[real_room(PRISONER_VAULT)].contents; olist; olist = olist->next_content)
    {
      if (GET_OBJ_VNUM(olist) == ovnum)
      {
        loaded = TRUE; /* this item was already loaded */
      }
    }

    /* this particular object is a valid number and hasn't been loaded, lets load it! */
    if (ovnum >= VALID_VNUM_LOW && ovnum <= VALID_VNUM_HiGH && loaded == FALSE)
    {
      /* check if there is a special handling for this particular item, such as loading numerous of the item instead of 1 */
      switch (ovnum)
      {
      case MITH_ARROW:
        obj_to_room(read_object(ovnum, VIRTUAL), real_room(PRISONER_VAULT));
        obj_to_room(read_object(ovnum, VIRTUAL), real_room(PRISONER_VAULT));
        obj_to_room(read_object(ovnum, VIRTUAL), real_room(PRISONER_VAULT));
        obj_to_room(read_object(ovnum, VIRTUAL), real_room(PRISONER_VAULT));
        obj_to_room(read_object(ovnum, VIRTUAL), real_room(PRISONER_VAULT));
        break;

      default:
        obj_to_room(read_object(ovnum, VIRTUAL), real_room(PRISONER_VAULT));
        break;
      }

      num_items++;
    }

    ovnum = NOTHING;
    loaded = FALSE;

    loop_counter++;

  } while (num_items < num_treasure && loop_counter < LOOP_LIMIT);

  /************************************************************************/
  /****** base items for treasury *************/

  /* create oil */
  struct obj_data *oil = read_object(WEAPON_OIL, VIRTUAL);
  if (oil)
  {
    GET_OBJ_VAL(oil, 0) = wpnOils[rand_number(0, TOP_UNIQUES_OIL)];
    obj_to_room(oil, real_room(PRISONER_VAULT));
  }

  /* potion */
  obj_to_room(read_object(LAVANDER_VIA, VIRTUAL), real_room(PRISONER_VAULT));

  /* weapon poison */
  obj_to_room(read_object(WEAPON_POISON, VIRTUAL), real_room(PRISONER_VAULT));

  /* coinage */
  obj_to_room(read_object(COINS_GOLD, VIRTUAL), real_room(PRISONER_VAULT));
  obj_to_room(read_object(COINS_PLAT, VIRTUAL), real_room(PRISONER_VAULT));
  obj_to_room(read_object(COINS_SILV, VIRTUAL), real_room(PRISONER_VAULT));

  /* random treasure, it'll be put on the lich */
  award_magic_item(NUM_TREASURE, ch, GRADE_SUPERIOR);
  /************************************************************************/

  /* the work is done! */
  return;
}
/*************************************/
/****** prisoner gear UNdefines! *******/
/*************************************/
/* unique */
#undef MALEVOLENCE
#undef CELESTIAL_SWRD
#undef HELL_SWRD
#undef MAGI_STAFF
#undef MOONBLADE
#undef DROW_SCIMITAR
#undef CRYSTAL_RAPIER
#undef STAR_CRICLET
#undef HOLY_PLATE
#undef DRAGONBONE_PLATE
#undef SPEED_GAUNT
#undef SHADOW_CLOAK
#undef ELVEN_CLOAK
#undef RUNED_QUIVER
#undef SLAADI_GOGS
#undef MANDRAKE_EAR
#undef MITH_ARROW
#undef ARM_VALOR
#undef BLACK_FIGURINE
/* base items */
#undef LAVANDER_VIA
#undef COINS_GOLD
#undef COINS_PLAT
#undef COINS_SILV
#undef WEAPON_OIL
/*************************************/
/* variables */
#undef TOP_UNIQUES
#undef VALID_VNUM_LOW
#undef VALID_VNUM_HiGH
#undef PRISONER_VAULT
#undef NUM_TREASURE
#undef LOOP_LIMIT
/*************************************/
/*************************************/

/* the prisoner primary form includes 5 heads, once those are defeated, then you get to fight
   the dracolich form */
SPECIAL(the_prisoner)
{

  if (cmd)
    return 0;

  /* make sure he has all 5 heads at the start of the battle (-1 indicates not killed) */
  if (prisoner_heads == -1)
    prisoner_heads = 5;

  /* this is the prisoner's regular form offensive arsenal */
  if (FIGHTING(ch))
    prisoner_attacks(ch);

  /* this is the prisoner's regular form defensive arsenal */
  if (!rand_number(0, 1))
  {
    if (rejuv_prisoner(ch))
    {
      return 1;
    }
  }

  return 0;
}

/* this is the final form of the prisoner! */
SPECIAL(dracolich)
{
  struct char_data *vict = NULL;
  int hitpoints = 0, use_aoe = 0;

  if (!ch)
    return 0;

  /* we have hit the gear creation section */
  /* we are checking 1) the items haven't loaded and 2) that the prisoner is engaged in combat to trigger this section */
  if (eq_loaded == FALSE && FIGHTING(ch))
  {
    prisoner_gear_loading(ch);
    eq_loaded = TRUE;
  }

  /* note that the !vict is moved below */
  if (cmd)
    return 0;

  if (!rand_number(0, 6))
  {

    /* find random target, and num targets */
    if (!(vict = npc_find_target(ch, &use_aoe)))
      return 0;

    act("\tLThe Prisoner cackles with glee at the fray, enjoying every second of the battle\r\n"
        "\tLShe sets her gaze upon you with the most wicked grin you have ever known.",
        FALSE, ch, 0, vict, TO_VICT);
    act("\tWAAAHHHH! You SCREAM in agony, a pain more intense than you have ever felt!\r\n"
        "\tWAs you fall, you see a stream of your own life force flowing away from you..",
        FALSE, ch, 0, vict, TO_VICT);
    act("\tLAs the life fades from your body, before collapsing you see is the Prisoner's wicked grin staring into your soul..\tn",
        FALSE, ch, 0, vict, TO_VICT);
    act("$n \tLturns and gazes at \tn$N\tL, who freezes in place.\tn\r\n"
        "$n \tLreaches out with a skeletal hand and touches \tn$N\tL!\tn",
        TRUE, ch, 0, vict, TO_NOTVICT);
    act("\tL$N\tr SCREAMS\tL in agony, doubling over in pain so intense it makes you cringe!!\tn\r\n"
        "$n\tL literally sucks the life force from $N,\tn\r\n"
        "\tLwho crumples into a ball of unfathomable pain onto the ground...\tn",
        TRUE, ch, 0, vict, TO_NOTVICT);
    act("\tWWith a grin, you whisper, 'die' at $N, who keels over and falls incapacitated!\tn", TRUE, ch, 0, vict,
        TO_CHAR);

    /* added a way to reduce the effectiveness of this attack -zusuk */
    if (AFF_FLAGGED(vict, AFF_DEATH_WARD) && !rand_number(0, 2))
    {
      hitpoints = damage(ch, vict, rand_number(120, 650), -1, DAM_UNHOLY, FALSE); // type -1 = no dam message
    }
    else
    {
      hitpoints = GET_HIT(vict);

      GET_HIT(vict) = 0;
    }

    if (hitpoints < 120)
      hitpoints = 120;

    if (GET_HIT(ch) + hitpoints < GET_MAX_HIT(ch))
      GET_HIT(ch) += hitpoints;

    return 1;
  }
  else if (!rand_number(0, 2))
  {
    prisoner_attacks(ch);
  }

  return 0;
}

/**********************/
/* End 'the Prisoner' */
/**********************/

/*****************/
/* Jot           */
/*****************/

#define JOT_VNUM 1960

#define MAX_FG 60    // fire giants
#define MAX_SB 20    // smoking beard batallion
#define MAX_EM 20    // efreeti mercenaries
#define MAX_FROST 65 // frost giants

bool jot_inv_check = false;

ACMD_DECL(do_say);

/* just made this to help facilitate switching of zone vnums if needed */
int jot_converter(int value)
{
  return (JOT_VNUM * 100) + value;
}

/* currently unused */
void jot_invasion()
{
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
    204, 204, 204, 204, 196};

/* load rooms for smoking beard batallion */
int sb_pos[MAX_SB] = {
    295, 295, 295, 295, 295,
    295, 295, 295, 295, 295,
    295, 295, 295, 215, 215,
    188, 188, 217, 206, 206};

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
    254, 256, 256, 243, 243};

/* spec proc for loading the jot invasion */
SPECIAL(jot_invasion_loader)
{
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
  for (tch = world[ch->in_room].people; tch; tch = chmove)
  {
    chmove = tch->next_in_room;
    /* glammad */
    if (GET_MOB_VNUM(tch) == jot_converter(80))
    {
      if ((roomrnum = real_room(jot_converter(204))) != NOWHERE)
      {
        glammad = tch; /* going to use this to form a group */
        char_from_room(glammad);
        char_to_room(glammad, roomrnum);
        if (!GROUP(glammad))
          create_group(glammad);
      }
    }
    /* fire giant captain(s) */
    if (GET_MOB_VNUM(tch) == jot_converter(81))
    {
      if ((roomrnum = real_room(jot_converter(204))) != NOWHERE)
      {
        char_from_room(tch);
        char_to_room(tch, roomrnum);
      }
    }
    /* sirthon quilen */
    if (GET_MOB_VNUM(tch) == jot_converter(83))
    {
      if ((roomrnum = real_room(jot_converter(115))) != NOWHERE)
      {
        char_from_room(tch);
        char_to_room(tch, roomrnum);
      }
    }
  }

  /* soldiers to glammad */
  for (i = 0; i < 2; i++)
  {
    mob = read_mobile(jot_converter(78), VIRTUAL);
    obj = read_object(jot_converter(17), VIRTUAL);
    if (obj && mob)
    {
      obj_to_char(obj, mob);
      perform_wield(mob, obj, TRUE);
      if ((roomrnum = real_room(jot_converter(204))) != NOWHERE)
      {
        char_to_room(mob, roomrnum);
        SET_BIT_AR(MOB_FLAGS(mob), MOB_SENTINEL);
        REMOVE_BIT_AR(MOB_FLAGS(mob), MOB_LISTEN);
        if (glammad)
        {
          add_follower(mob, glammad);
          join_group(mob, GROUP(glammad));
        }
      }
    }
  }

  /* twilight to treasure room */
  if ((objrnum = real_object(jot_converter(90))) != NOWHERE)
  {
    if ((obj = read_object(objrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(296))) != NOWHERE)
      {
        obj_to_room(obj, roomrnum);
      }
    }
  }
  /* fire giant crown to treasure room */
  if ((objrnum = real_object(jot_converter(82))) != NOWHERE)
  {
    if ((obj = read_object(objrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(296))) != NOWHERE)
      {
        obj_to_room(obj, roomrnum);
      }
    }
  }

  /* extra jarls to deal with */
  for (i = 0; i < 2; i++)
  { /* treasure room */
    if ((mobrnum = real_mobile(jot_converter(39))) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(jot_converter(296))) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
        }
      }
    }
  }
  for (i = 0; i < 3; i++)
  { /* uthgard loki throne room */
    if ((mobrnum = real_mobile(jot_converter(39))) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(jot_converter(287))) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
        }
      }
    }
  }

  /* heavily guarded gatehouse, frost giant mage is leading this group */
  if ((mobrnum = real_mobile(jot_converter(90))) != NOBODY)
  {
    if ((leader = read_mobile(mobrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(266))) != NOWHERE)
      {
        char_to_room(leader, roomrnum);
        if (!GROUP(leader))
          create_group(leader);
      }
    }
  }
  /* 2nd mage in group */
  if ((mobrnum = real_mobile(jot_converter(90))) != NOBODY)
  {
    if ((mob = read_mobile(mobrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(266))) != NOWHERE)
      {
        char_to_room(mob, roomrnum);
        if (leader)
        {
          add_follower(mob, leader);
          join_group(mob, GROUP(leader));
        }
      }
    }
  }
  /* citadel guards join the group */
  if (roomrnum != NOWHERE)
  {
    for (i = 0; i < 8; i++)
    {
      mob = read_mobile(jot_converter(33), VIRTUAL);
      obj = read_object(jot_converter(28), VIRTUAL);
      if (mob && obj)
      {
        obj_to_char(obj, mob);
        perform_wield(mob, obj, TRUE);
      }
      if ((obj2 = read_object(jot_converter(41), VIRTUAL)) != NULL)
      {
        obj_to_char(obj2, mob);
        where = find_eq_pos(mob, obj2, 0);
        perform_wear(mob, obj2, where);
      }
      char_to_room(mob, roomrnum);
      if (leader)
      {
        add_follower(mob, leader);
        join_group(mob, GROUP(leader));
      }
    }
  }

  /* large gatehouse group, led by a mage again */
  if ((mobrnum = real_mobile(jot_converter(90))) != NOBODY)
  {
    if ((leader = read_mobile(mobrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(252))) != NOWHERE)
      {
        char_to_room(leader, roomrnum);
        if (!GROUP(leader))
          create_group(leader);
      }
    }
  }
  /* 2nd mage in group */
  if ((mobrnum = real_mobile(jot_converter(90))) != NOBODY)
  {
    if ((mob = read_mobile(mobrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(252))) != NOWHERE)
      {
        char_to_room(mob, roomrnum);
        if (leader)
        {
          add_follower(mob, leader);
          join_group(mob, GROUP(leader));
        }
      }
    }
  }
  /* citadel guards join the group */
  if (roomrnum != NOWHERE)
  {
    for (i = 0; i < 5; i++)
    {
      mob = read_mobile(jot_converter(33), VIRTUAL);
      obj = read_object(jot_converter(28), VIRTUAL);
      if (mob && obj)
      {
        obj_to_char(obj, mob);
        perform_wield(mob, obj, TRUE);
      }
      if ((obj2 = read_object(jot_converter(40), VIRTUAL)) != NULL)
      {
        obj_to_char(obj2, mob);
        where = find_eq_pos(mob, obj2, 0);
        perform_wear(mob, obj2, where);
      }
      char_to_room(mob, roomrnum);
      if (leader)
      {
        add_follower(mob, leader);
        join_group(mob, GROUP(leader));
      }
    }
  }

  /* load up some firegiants, then equip them */
  for (i = 0; i < MAX_FG; i++)
  {
    if ((roomrnum = real_room(jot_converter(fg_pos[i]))) != NOWHERE)
    {
      if ((mob = read_mobile(jot_converter(78), VIRTUAL)) != NULL)
      {
        char_to_room(mob, roomrnum);
        if ((obj = read_object(jot_converter(17), VIRTUAL)) != NULL)
        {
          obj_to_char(obj, mob);
          perform_wield(mob, obj, TRUE);
        }
      }
    }
  }

  /* load up smoking beard batallion */
  for (i = 0; i < MAX_SB; i++)
  {
    if ((roomrnum = real_room(jot_converter(sb_pos[i]))) != NOWHERE)
    {
      if ((mob = read_mobile(jot_converter(79), VIRTUAL)) != NULL)
      {
        char_to_room(mob, roomrnum);
      }
    }
  }

  /* efreeti mercenary */
  for (i = 0; i < MAX_EM; i++)
  {
    if ((roomrnum = real_room(jot_converter(295))) != NOWHERE)
    {
      if ((mob = read_mobile(jot_converter(84), VIRTUAL)) != NULL)
      {
        char_to_room(mob, roomrnum);
      }
    }
  }

  /* Extra frost giants */
  for (i = 0; i < MAX_FROST; i++)
  {
    if ((roomrnum = real_room(jot_converter(frost_pos[i]))) != NOWHERE)
    {
      if ((mob = read_mobile(jot_converter(85), VIRTUAL)) != NULL)
      {
        char_to_room(mob, roomrnum);
        if ((obj = read_object(jot_converter(28), VIRTUAL)) != NULL)
        {
          obj_to_char(obj, mob);
          perform_wield(mob, obj, TRUE);
        }
      }
    }
  }

  /* Valkyrie */
  if ((roomrnum = real_room(jot_converter(4))) != NOWHERE)
  {
    if ((mob = read_mobile(jot_converter(82), VIRTUAL)) != NULL)
    {
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

SPECIAL(thrym)
{
  if (!ch)
    return 0;

  struct char_data *vict = FIGHTING(ch);
  struct affected_type af;

  if (cmd || !vict || rand_number(0, 8))
    return 0;

  if (paralysis_immunity(vict))
  {
    send_to_char(ch, "Your target is unfazed.\r\n");
    return 1;
  }

  // no save, unless have special feat
  if (HAS_FEAT(vict, FEAT_PARALYSIS_RESIST))
  {
    mag_savingthrow(ch, vict, SAVING_FORT, +4, /* +4 bonus from feat */
                    CAST_INNATE, 30, ENCHANTMENT);
    send_to_char(ch, "Your target is unfazed.\r\n");
    return 1;
  }

  act("\tCThrym touches you with a chilling hand, freezing you in place.\tn", FALSE, vict, 0, ch, TO_CHAR);
  act("\tCThrym touches $n\tC, freezing $m in place.\tn", FALSE, vict, 0, ch, TO_ROOM);

  new_affect(&af);
  af.spell = SPELL_HOLD_PERSON;
  SET_BIT_AR(af.bitvector, AFF_PARALYZED);
  af.duration = 8;
  affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);

  return 1;
}

SPECIAL(ymir)
{
  if (!ch || cmd)
    return 0;

  if (FIGHTING(ch) && !rand_number(0, 4))
  {
    call_magic(ch, FIGHTING(ch), 0, SPELL_FROST_BREATHE, 0, GET_LEVEL(ch), CAST_INNATE);
    return 1;
  }

  return 0;
}

SPECIAL(planetar)
{
  if (!ch || cmd)
    return 0;

  if (FIGHTING(ch) && !rand_number(0, 5))
  {
    call_magic(ch, FIGHTING(ch), 0, SPELL_LIGHTNING_BREATHE, 0, GET_LEVEL(ch), CAST_INNATE);
    return 1;
  }

  return 0;
}

SPECIAL(gatehouse_guard)
{
  struct char_data *mob = (struct char_data *)me;

  if (!IS_MOVE(cmd) || AFF_FLAGGED(mob, AFF_BLIND) || AFF_FLAGGED(mob, AFF_SLEEP) ||
      AFF_FLAGGED(mob, AFF_PARALYZED) || AFF_FLAGGED(mob, AFF_GRAPPLED) ||
      AFF_FLAGGED(mob, AFF_ENTANGLED) || HAS_WAIT(mob))
    return FALSE;

  if (cmd == SCMD_EAST && (!IS_NPC(ch) || IS_PET(ch)) && GET_LEVEL(ch) < 31)
  {
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
SPECIAL(ymir_cloak)
{
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke ice storm by saying 'icicle storm'.\r\nOnce per day.\r\n");
    return 1;
  }

  struct obj_data *obj = (struct obj_data *)me;

  if (cmd && argument && CMD_IS("say"))
  {
    if (!is_wearing(ch, 196059))
      return 0;

    skip_spaces(&argument);

    if (!strcmp(argument, "icicle storm"))
    {

      if (GET_OBJ_SPECTIMER(obj, 0) > 0)
      {
        send_to_char(ch, "\tcAs you say '\tCicicle storm\tc' to your \tWa cloak of glittering icicles\tc, nothing happens.\tn\r\n");
        return 1;
      }

      weapons_spells("\tBAs you say '\twicicle storm\tB' to $p \tBit flashes bright blue and sends forth a storm of razor sharp icicles in all directions.\tn",
                     "\tBAs $n \tBmutters something under his breath  to $p \tBit flashes bright blue and sends forth a storm of razor sharp icicles in all directions.\tn",
                     "\tBAs $n \tBmutters something under his breath  to $p \tBit flashes bright blue and sends forth a storm of razor sharp icicles in all directions.\tn",
                     ch, 0, (struct obj_data *)me, SPELL_ICE_STORM);
      GET_OBJ_SPECTIMER(obj, 0) = 6;
      return 1;
    }
  }
  return 0;
}

/* mistweave mace object proc */
SPECIAL(mistweave)
{
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke blindness by saying 'mistweave'. Once per day.\r\n");
    return 1;
  }

  struct obj_data *obj = (struct obj_data *)me;
  struct char_data *vict = FIGHTING(ch);

  if (cmd && argument && CMD_IS("say"))
  {
    if (!is_wearing(ch, 196012))
      return 0;

    skip_spaces(&argument);

    if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
        !strcmp(argument, "mistweave"))
    {

      if (GET_OBJ_SPECTIMER(obj, 0) > 0)
      {
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

      call_magic(ch, FIGHTING(ch), 0, SPELL_BLINDNESS, 0, 30, CAST_WEAPON_SPELL);
      GET_OBJ_SPECTIMER(obj, 0) = 24;
      return 1;
    }
    else
      return 0;
  }
  return 0;
}

/* frostbite axe proc */
SPECIAL(frostbite)
{
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke cone of cold  by saying 'frostbite'. Once per day.\r\n");
    return 1;
  }

  struct obj_data *obj = (struct obj_data *)me;
  struct char_data *vict = FIGHTING(ch);
  int pct;
  struct affected_type af;

  if (cmd && argument && CMD_IS("say"))
  {
    if (!is_wearing(ch, 196000))
      return 0;

    skip_spaces(&argument);

    if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
        !strcmp(argument, "frostbite"))
    {

      if (GET_OBJ_SPECTIMER(obj, 0) > 0)
      {
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
        call_magic(ch, vict, 0, SPELL_CONE_OF_COLD, 0, 20, CAST_WEAPON_SPELL);
      else if (pct < 85)
        call_magic(ch, vict, 0, SPELL_CONE_OF_COLD, 0, 30, CAST_WEAPON_SPELL);
      else
      {
        call_magic(ch, vict, 0, SPELL_CONE_OF_COLD, 0, 30, CAST_WEAPON_SPELL);
        if (!paralysis_immunity(vict))
        {
          new_affect(&af);
          af.spell = SPELL_HOLD_PERSON;
          SET_BIT_AR(af.bitvector, AFF_PARALYZED);
          af.duration = dice(2, 4);
          affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);
        }
      }

      GET_OBJ_SPECTIMER(obj, 0) = 24;
      return 1;
    }
    else
      return 0;
  }
  return 0;
}

/* special claws gear with proc */
#define VAP_AFFECTS 3

SPECIAL(vaprak_claws)
{
  struct affected_type af[VAP_AFFECTS];
  int duration = 0, i = 0;
  struct obj_data *obj = (struct obj_data *)me;

  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke Fury of Vaprak by saying 'vaprak'. Once per day.\r\nWorks only for Trolls and Ogres.\r\n");
    return 1;
  }

  if (!argument)
    return 0;

  /*
  if (GET_RACE(ch) != RACE_OGRE && GET_RACE(ch) != RACE_HALF_TROLL)
    return 0;
   */
  if (GET_RACE(ch) != RACE_HALF_TROLL)
    return 0;

  if (!is_wearing(ch, 196062))
    return 0;

  skip_spaces(&argument);

  if (!strcmp(argument, "vaprak") && CMD_IS("say"))
  {

    // if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)) {
    if (GET_OBJ_SPECTIMER(obj, 0) > 0)
    {
      send_to_char(ch, "\trAs you say '\twvaprak\tr' to your claws \tLof the destroyer\tr, nothing happens.\tn\r\n");
      return 1;
    }

    if (affected_by_spell(ch, SKILL_RAGE) || affected_by_spell(ch, SKILL_DEFENSIVE_STANCE))
    {
      send_to_char(ch, "You are already raging or in a defensive stance!\r\n");
      return 1;
    }

    weapons_spells("\tLAs you say '\twvaprak\tL' to $p\tL, an evil warmth fills your body.\tn",
                   0,
                   "\tr$n \trmutters something under his breath.\tn",
                   ch, ch, (struct obj_data *)me, 0);

    duration = GET_LEVEL(ch);
    /* init affect array */
    for (i = 0; i < VAP_AFFECTS; i++)
    {
      new_affect(&(af[i]));
      af[i].spell = SKILL_RAGE;
      af[i].duration = duration;
    }

    af[0].location = APPLY_HITROLL;
    af[0].modifier = 3;

    af[1].location = APPLY_DAMROLL;
    af[1].modifier = 3;

    af[2].location = APPLY_SAVING_WILL;
    af[2].modifier = 3;
    SET_BIT_AR(af[2].bitvector, AFF_HASTE);

    for (i = 0; i < VAP_AFFECTS; i++)
      affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);
    GET_OBJ_SPECTIMER(obj, 0) = 24;
    return 1; /* success! */
  }

  return 0;
}
#undef VAP_AFFECTS

/* a fake twilight proc (large sword) */
#define TWI_AFFECTS 2

SPECIAL(fake_twilight)
{
  struct affected_type af[TWI_AFFECTS];
  struct char_data *vict;
  int duration = 0, i = 0;

  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Twilight Rage.\r\n");
    return 1;
  }

  vict = FIGHTING(ch);

  if (affected_by_spell(ch, SPELL_BATTLETIDE))
  {
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
                 ch, vict, (struct obj_data *)me, 0);

  duration = GET_LEVEL(ch) / 5;
  /* init affect array */
  for (i = 0; i < TWI_AFFECTS; i++)
  {
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
SPECIAL(twilight)
{
  struct affected_type af[TWI_AFFECTS];
  struct char_data *vict;
  int duration = 0, i = 0;

  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Twilight Rage!\r\n");
    return 1;
  }

  vict = FIGHTING(ch);

  if (affected_by_spell(ch, SPELL_BATTLETIDE))
  {
    return 0;
  }

  if (cmd || !vict || rand_number(0, 12))
    return 0;

  weapons_spells("\tLA glimmer of insanity crosses your face as your\r\n"
                 "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
                 "\tLA glimmer of insanity crosses $n\tL's face as $s\r\n"
                 "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
                 "\tLA glimmer of insanity crosses $n\tL's face as $s\r\n"
                 "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
                 ch, vict, (struct obj_data *)me, 0);

  duration = GET_LEVEL(ch) / 5 + 1;
  /* init affect array */
  for (i = 0; i < TWI_AFFECTS; i++)
  {
    new_affect(&(af[i]));
    af[i].spell = SPELL_BATTLETIDE;
    af[i].duration = duration;
  }

  af[0].location = APPLY_HITROLL;
  af[0].modifier = GET_STR_BONUS(ch);
  af[0].bonus_type = BONUS_TYPE_CIRCUMSTANCE;

  af[1].location = APPLY_DAMROLL;
  af[1].modifier = GET_STR_BONUS(ch);
  af[1].bonus_type = BONUS_TYPE_CIRCUMSTANCE;

  for (i = 0; i < TWI_AFFECTS; i++)
    affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);

  return 1;
}
#undef TWI_AFFECTS

SPECIAL(valkyrie_sword)
{

  if (!ch || cmd)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Female Only - Proc Burning Hands\r\n");
    return 1;
  }

  if (GET_SEX(ch) != SEX_FEMALE && !IS_NPC(ch))
  {
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
      "\tYengulfing $n's \tYfoe.",
      ch, vict, (struct obj_data *)me, 0);

  call_magic(ch, vict, 0, SPELL_BURNING_HANDS, 0, 30, CAST_WEAPON_SPELL);

  return 1;
}

SPECIAL(planetar_sword)
{
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc Cure Critic and Dispel Evil\r\n");
    return 1;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 27))
    return 0;

  switch (rand_number(0, 1))
  {
  case 1:
    weapons_spells("\tWA nimbus of holy light surrounds your sword, bathing you in its radiance\tn",
                   0,
                   "\tWA nimbus of holy light surrounds $n's\tW sword, bathing $m in its radiance.",
                   ch, ch, (struct obj_data *)me, SPELL_CURE_CRITIC);
    call_magic(ch, ch, 0, SPELL_CURE_CRITIC, 0, GET_LEVEL(ch), CAST_WEAPON_SPELL);
    return 1;
  case 2:
    weapons_spells("\tWA glowing nimbus of light emanates forth blasting the foul evil in its presence.\tn",
                   "\tWA glowing nimbus of light emanates forth from $n, blasting the foul evil in its presence.\tn",
                   "\tWA glowing nimbus of light emanates forth from $n, blasting the foul evil in its presence.\tn",
                   ch, vict, (struct obj_data *)me, SPELL_DISPEL_EVIL);
    return 1;
  default:
    return 0;
  }

  return 1;
}

SPECIAL(giantslayer)
{
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke giant hamstring attack by saying 'hamstring'. Once per day.\r\nWorks only for Dwarves.\r\n");
    return 1;
  }

  switch (GET_RACE(ch))
  {

  case RACE_DWARF:
    break;

  case RACE_DUERGAR:
    break;

  default:
    return 0;
    break;
  }

  struct obj_data *obj = (struct obj_data *)me;
  struct char_data *vict = FIGHTING(ch);

  if (!vict)
    return 0;

  skip_spaces(&argument);
  if (!is_wearing(ch, 196066))
    return 0;
  if (!strcmp(argument, "hamstring"))
  {
    if (IS_NPC(vict) && GET_RACE(vict) == RACE_TYPE_GIANT &&
        (vict->in_room == ch->in_room))
    {
      if (GET_OBJ_SPECTIMER(obj, 0) > 0)
      {
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
      USE_MOVE_ACTION(vict);
      change_position(vict, POS_SITTING);
      GET_HIT(vict) -= 100;

      GET_OBJ_SPECTIMER(obj, 0) = 24;
      return 1; // end for
    }
    else
    {
      send_to_char(ch, "\tYAs you say '\twhamstring\tY' to your \tLa double-bladed dwarvish axe of \tYgiantslaying, nothing happens.\tn\r\n");
      return 1;
    }
    return 0;
  }
  return 0;
}

#undef JOT_VNUM
#undef MAX_FG    // fire giants
#undef MAX_SB    // smoking beard batallion
#undef MAX_EM    // efreeti mercenaries
#undef MAX_FROST // frost giants

/*****************/
/* End Jot       */
/*****************/

/*****************/
/* Mad Drow */
/*****************/

bool open_msg = FALSE;
bool close_msg = FALSE;

struct slider_row
{
  int room;
  int door;
};

struct slider_row row_1_a_n_s[] = {
    {155521, EAST},
    {155522, WEST},
    {155530, EAST},
    {155529, WEST},
    {155531, EAST},
    {155532, WEST},
    {155540, EAST},
    {155539, WEST},
    {155541, EAST},
    {155542, WEST},
    {-1, -1}};
struct slider_row row_1_b_n_s[] = {
    {155546, EAST},
    {155547, WEST},
    {155555, EAST},
    {155554, WEST},
    {155556, EAST},
    {155557, WEST},
    {155565, EAST},
    {155564, WEST},
    {155566, EAST},
    {155567, WEST},
    {-1, -1}};
struct slider_row row_1_c_n_s[] = {
    {155571, EAST},
    {155572, WEST},
    {155580, EAST},
    {155579, WEST},
    {155581, EAST},
    {155582, WEST},
    {155590, EAST},
    {155589, WEST},
    {155591, EAST},
    {155592, WEST},
    {-1, -1}};
struct slider_row row_1_d_n_s[] = {
    {155596, EAST},
    {155597, WEST},
    {155605, EAST},
    {155604, WEST},
    {155606, EAST},
    {155607, WEST},
    {155615, EAST},
    {155614, WEST},
    {155616, EAST},
    {155617, WEST},
    {-1, -1}};
struct slider_row row_1_e_n_s[] = {
    {155625, EAST},
    {155624, WEST},
    {155626, EAST},
    {155627, WEST},
    {155635, EAST},
    {155634, WEST},
    {155636, EAST},
    {155637, WEST},
    {155645, EAST},
    {155644, WEST},
    {-1, -1}};
struct slider_row row_2_a_n_s[] = {
    {155522, EAST},
    {155523, WEST},
    {155529, EAST},
    {155528, WEST},
    {155532, EAST},
    {155533, WEST},
    {155539, EAST},
    {155538, WEST},
    {155542, EAST},
    {155543, WEST},
    {-1, -1}};
struct slider_row row_2_b_n_s[] = {
    {155547, EAST},
    {155548, WEST},
    {155554, EAST},
    {155553, WEST},
    {155557, EAST},
    {155558, WEST},
    {155564, EAST},
    {155563, WEST},
    {155567, EAST},
    {155568, WEST},
    {-1, -1}};
struct slider_row row_2_c_n_s[] = {
    {155572, EAST},
    {155573, WEST},
    {155579, WEST},
    {155578, WEST},
    {155582, EAST},
    {155583, WEST},
    {155589, EAST},
    {155588, WEST},
    {155592, EAST},
    {155593, WEST},
    {-1, -1}};
struct slider_row row_2_d_n_s[] = {
    {155597, EAST},
    {155598, WEST},
    {155604, EAST},
    {155603, WEST},
    {155607, EAST},
    {155608, WEST},
    {155614, EAST},
    {155613, WEST},
    {155617, EAST},
    {155618, WEST},
    {-1, -1}};
struct slider_row row_2_e_n_s[] = {
    {155624, EAST},
    {155623, WEST},
    {155627, EAST},
    {155628, WEST},
    {155634, EAST},
    {155633, WEST},
    {155637, EAST},
    {155638, WEST},
    {155644, EAST},
    {155643, WEST},
    {-1, -1}};
struct slider_row row_3_a_n_s[] = {
    {155523, EAST},
    {155534, WEST},
    {155528, EAST},
    {155527, WEST},
    {155533, EAST},
    {155534, WEST},
    {155538, EAST},
    {155537, WEST},
    {155543, EAST},
    {155544, WEST},
    {-1, -1}};
struct slider_row row_3_b_n_s[] = {
    {155548, EAST},
    {155549, WEST},
    {155553, EAST},
    {155552, WEST},
    {155559, EAST},
    {155559, WEST},
    {155563, EAST},
    {155562, WEST},
    {155568, EAST},
    {155569, WEST},
    {-1, -1}};
struct slider_row row_3_c_n_s[] = {
    {155573, EAST},
    {155574, WEST},
    {155578, EAST},
    {155577, WEST},
    {155583, EAST},
    {155584, WEST},
    {155588, EAST},
    {155587, WEST},
    {155593, EAST},
    {155594, WEST},
    {-1, -1}};
struct slider_row row_3_d_n_s[] = {
    {155598, EAST},
    {155599, WEST},
    {155603, EAST},
    {155602, WEST},
    {155608, EAST},
    {155609, WEST},
    {155613, EAST},
    {155612, WEST},
    {155618, EAST},
    {155619, WEST},
    {-1, -1}};
struct slider_row row_3_e_n_s[] = {
    {155623, EAST},
    {155622, WEST},
    {155628, EAST},
    {155629, WEST},
    {155633, EAST},
    {155632, WEST},
    {155638, EAST},
    {155639, WEST},
    {155643, EAST},
    {155642, WEST},
    {-1, -1}};
struct slider_row row_4_a_n_s[] = {
    {155524, EAST},
    {155525, WEST},
    {155527, EAST},
    {155526, WEST},
    {155534, EAST},
    {155535, WEST},
    {155537, EAST},
    {155536, WEST},
    {155544, EAST},
    {155545, WEST},
    {-1, -1}};
struct slider_row row_4_b_n_s[] = {
    {155549, EAST},
    {155550, WEST},
    {155552, EAST},
    {155551, WEST},
    {155559, EAST},
    {155560, WEST},
    {155562, EAST},
    {155561, WEST},
    {155569, EAST},
    {155570, WEST},
    {-1, -1}};
struct slider_row row_4_c_n_s[] = {
    {155574, EAST},
    {155575, WEST},
    {155577, EAST},
    {155576, WEST},
    {155584, EAST},
    {155585, WEST},
    {155587, EAST},
    {155586, WEST},
    {155594, EAST},
    {155595, WEST},
    {-1, -1}};
struct slider_row row_4_d_n_s[] = {
    {155599, EAST},
    {155600, WEST},
    {155602, EAST},
    {155601, WEST},
    {155609, EAST},
    {155610, WEST},
    {155612, EAST},
    {155611, WEST},
    {155619, EAST},
    {155620, WEST},
    {-1, -1}};
struct slider_row row_4_e_n_s[] = {
    {155622, EAST},
    {155621, WEST},
    {155629, EAST},
    {155630, WEST},
    {155632, EAST},
    {155631, WEST},
    {155632, EAST},
    {155631, WEST},
    {155639, EAST},
    {155640, WEST},
    {155642, EAST},
    {155641, WEST},
    {-1, -1}};
struct slider_row row_1_a_e_w[] = {
    {155521, SOUTH},
    {155530, NORTH},
    {155522, SOUTH},
    {155523, SOUTH},
    {155524, SOUTH},
    {155525, SOUTH},
    {155529, NORTH},
    {155528, NORTH},
    {155527, NORTH},
    {155526, NORTH},
    {-1, -1}};
struct slider_row row_1_b_e_w[] = {
    {155546, SOUTH},
    {155547, SOUTH},
    {155548, SOUTH},
    {155549, SOUTH},
    {155550, SOUTH},
    {155555, NORTH},
    {155554, NORTH},
    {155553, NORTH},
    {155552, NORTH},
    {155551, NORTH},
    {-1, -1}};
struct slider_row row_1_c_e_w[] = {
    {155571, SOUTH},
    {155572, SOUTH},
    {155573, SOUTH},
    {155574, SOUTH},
    {155575, SOUTH},
    {155580, NORTH},
    {155579, NORTH},
    {155578, NORTH},
    {155577, NORTH},
    {155576, NORTH},
    {-1, -1}};
struct slider_row row_1_d_e_w[] = {
    {155596, SOUTH},
    {155597, SOUTH},
    {155598, SOUTH},
    {155599, SOUTH},
    {155600, SOUTH},
    {155605, NORTH},
    {155604, NORTH},
    {155603, NORTH},
    {155602, NORTH},
    {155601, NORTH},
    {-1, -1}};
struct slider_row row_1_e_e_w[] = {
    {155625, SOUTH},
    {155624, SOUTH},
    {155623, SOUTH},
    {155622, SOUTH},
    {155621, SOUTH},
    {155626, NORTH},
    {155627, NORTH},
    {155628, NORTH},
    {155629, NORTH},
    {155630, NORTH},
    {-1, -1}};
struct slider_row row_2_a_e_w[] = {
    {155530, SOUTH},
    {155531, NORTH},
    {155529, SOUTH},
    {155527, SOUTH},
    {155526, SOUTH},
    {155532, NORTH},
    {155533, NORTH},
    {155534, NORTH},
    {155535, NORTH},
    {-1, -1}};
struct slider_row row_2_b_e_w[] = {
    {155555, SOUTH},
    {155554, SOUTH},
    {155553, SOUTH},
    {155552, SOUTH},
    {155551, SOUTH},
    {155556, NORTH},
    {155557, NORTH},
    {155558, NORTH},
    {155559, NORTH},
    {155560, NORTH},
    {-1, -1}};
struct slider_row row_2_c_e_w[] = {
    {155580, SOUTH},
    {155579, SOUTH},
    {155578, SOUTH},
    {155577, SOUTH},
    {155576, SOUTH},
    {155581, NORTH},
    {155582, NORTH},
    {155583, NORTH},
    {155584, NORTH},
    {155585, NORTH},
    {-1, -1}};
struct slider_row row_2_d_e_w[] = {
    {155605, SOUTH},
    {155604, SOUTH},
    {155603, SOUTH},
    {155602, SOUTH},
    {155601, SOUTH},
    {155606, NORTH},
    {155607, NORTH},
    {155608, NORTH},
    {155609, NORTH},
    {155610, NORTH},
    {-1, -1}};
struct slider_row row_2_e_e_w[] = {
    {155626, SOUTH},
    {155627, SOUTH},
    {155628, SOUTH},
    {155629, SOUTH},
    {155630, SOUTH},
    {155635, NORTH},
    {155634, NORTH},
    {155633, NORTH},
    {155632, NORTH},
    {155631, NORTH},
    {-1, -1}};
struct slider_row row_3_a_e_w[] = {
    {155531, SOUTH},
    {155540, NORTH},
    {155532, SOUTH},
    {155533, SOUTH},
    {155534, SOUTH},
    {155535, SOUTH},
    {155539, NORTH},
    {155538, NORTH},
    {155537, NORTH},
    {155536, NORTH},
    {-1, -1}};
struct slider_row row_3_b_e_w[] = {
    {155556, SOUTH},
    {155557, SOUTH},
    {155558, SOUTH},
    {155559, SOUTH},
    {155560, SOUTH},
    {155565, NORTH},
    {155564, NORTH},
    {155563, NORTH},
    {155562, NORTH},
    {155561, NORTH},
    {-1, -1}};
struct slider_row row_3_c_e_w[] = {
    {155581, SOUTH},
    {155582, SOUTH},
    {155583, SOUTH},
    {155584, SOUTH},
    {155585, SOUTH},
    {155590, NORTH},
    {155589, NORTH},
    {155588, NORTH},
    {155587, NORTH},
    {155586, NORTH},
    {-1, -1}};
struct slider_row row_3_d_e_w[] = {
    {155606, SOUTH},
    {155607, SOUTH},
    {155608, SOUTH},
    {155609, SOUTH},
    {155610, SOUTH},
    {155615, NORTH},
    {155614, NORTH},
    {155613, NORTH},
    {155612, NORTH},
    {155611, NORTH},
    {-1, -1}};
struct slider_row row_3_e_e_w[] = {
    {155635, SOUTH},
    {155634, SOUTH},
    {155633, SOUTH},
    {155632, SOUTH},
    {155631, SOUTH},
    {155636, NORTH},
    {155637, NORTH},
    {155638, NORTH},
    {155639, NORTH},
    {155640, NORTH},
    {-1, -1}};
struct slider_row row_4_a_e_w[] = {
    {155540, SOUTH},
    {155541, NORTH},
    {155539, SOUTH},
    {155538, SOUTH},
    {155537, SOUTH},
    {155536, SOUTH},
    {155542, NORTH},
    {155543, NORTH},
    {155544, NORTH},
    {155545, NORTH},
    {-1, -1}};
struct slider_row row_4_b_e_w[] = {
    {155565, SOUTH},
    {155564, SOUTH},
    {155563, SOUTH},
    {155562, SOUTH},
    {155561, SOUTH},
    {155566, NORTH},
    {155567, NORTH},
    {155568, NORTH},
    {155569, NORTH},
    {155570, NORTH},
    {-1, -1}};
struct slider_row row_4_c_e_w[] = {
    {155590, SOUTH},
    {155589, SOUTH},
    {155588, SOUTH},
    {155587, SOUTH},
    {155586, SOUTH},
    {155591, NORTH},
    {155592, NORTH},
    {155593, NORTH},
    {155594, NORTH},
    {155595, NORTH},
    {-1, -1}};
struct slider_row row_4_d_e_w[] = {
    {155615, SOUTH},
    {155614, SOUTH},
    {155613, SOUTH},
    {155612, SOUTH},
    {155611, SOUTH},
    {155616, NORTH},
    {155617, NORTH},
    {155618, NORTH},
    {155619, NORTH},
    {155620, NORTH},
    {-1, -1}};
struct slider_row row_4_e_e_w[] = {
    {155636, SOUTH},
    {155637, SOUTH},
    {155638, SOUTH},
    {155639, SOUTH},
    {155640, SOUTH},
    {155645, NORTH},
    {155644, NORTH},
    {155643, NORTH},
    {155642, NORTH},
    {155641, NORTH},
    {-1, -1}};
struct slider_row row_1_a_u_d[] = {
    {155521, DOWN},
    {155546, UP},
    {155530, DOWN},
    {155555, UP},
    {155531, DOWN},
    {155556, UP},
    {155540, DOWN},
    {155565, UP},
    {155541, DOWN},
    {155566, UP},
    {-1, -1}};
struct slider_row row_1_b_u_d[] = {
    {155522, DOWN},
    {155547, UP},
    {155529, DOWN},
    {155554, UP},
    {155532, DOWN},
    {155557, UP},
    {155539, DOWN},
    {155564, UP},
    {155542, DOWN},
    {155567, UP},
    {-1, -1}};
struct slider_row row_1_c_u_d[] = {
    {155523, DOWN},
    {155528, DOWN},
    {155533, DOWN},
    {155538, DOWN},
    {155543, DOWN},
    {155548, UP},
    {155553, UP},
    {155558, UP},
    {155563, UP},
    {155568, UP},
    {-1, -1}};
struct slider_row row_1_d_u_d[] = {
    {155524, DOWN},
    {155527, DOWN},
    {155534, DOWN},
    {155537, DOWN},
    {155544, DOWN},
    {155549, UP},
    {155552, UP},
    {155559, UP},
    {155562, UP},
    {155569, UP},
    {-1, -1}};
struct slider_row row_1_e_u_d[] = {
    {155525, DOWN},
    {155526, DOWN},
    {155535, DOWN},
    {155536, DOWN},
    {155545, DOWN},
    {155550, UP},
    {155551, UP},
    {155560, UP},
    {155561, UP},
    {155570, UP},
    {-1, -1}};
struct slider_row row_2_a_u_d[] = {
    {155546, DOWN},
    {155571, UP},
    {155555, DOWN},
    {155556, DOWN},
    {155565, DOWN},
    {155566, DOWN},
    {155580, UP},
    {155581, UP},
    {155590, UP},
    {155591, UP},
    {-1, -1}};
struct slider_row row_2_b_u_d[] = {
    {155547, DOWN},
    {155554, DOWN},
    {155557, DOWN},
    {155564, DOWN},
    {155567, DOWN},
    {155572, UP},
    {155569, UP},
    {155582, UP},
    {155589, UP},
    {155592, UP},
    {-1, -1}};
struct slider_row row_2_c_u_d[] = {
    {155548, DOWN},
    {155553, DOWN},
    {155558, DOWN},
    {155563, DOWN},
    {155568, DOWN},
    {155573, UP},
    {155578, UP},
    {155583, UP},
    {155588, UP},
    {155593, UP},
    {-1, -1}};
struct slider_row row_2_d_u_d[] = {
    {155549, DOWN},
    {155552, DOWN},
    {155559, DOWN},
    {155562, DOWN},
    {155569, DOWN},
    {155574, UP},
    {155577, UP},
    {155584, UP},
    {155587, UP},
    {155594, UP},
    {-1, -1}};
struct slider_row row_2_e_u_d[] = {
    {155550, DOWN},
    {155551, DOWN},
    {155560, DOWN},
    {155561, DOWN},
    {155570, DOWN},
    {155575, UP},
    {155576, UP},
    {155585, UP},
    {155586, UP},
    {155595, UP},
    {-1, -1}};
struct slider_row row_3_a_u_d[] = {
    {155571, DOWN},
    {155596, UP},
    {155580, DOWN},
    {155581, DOWN},
    {155590, DOWN},
    {155591, DOWN},
    {155596, UP},
    {155605, UP},
    {155606, UP},
    {155615, UP},
    {155616, UP},
    {-1, -1}};
struct slider_row row_3_b_u_d[] = {
    {155572, DOWN},
    {155579, DOWN},
    {155582, DOWN},
    {155589, DOWN},
    {155592, DOWN},
    {155597, UP},
    {155604, UP},
    {155607, UP},
    {155614, UP},
    {155617, UP},
    {-1, -1}};
struct slider_row row_3_c_u_d[] = {
    {155573, DOWN},
    {155578, DOWN},
    {155583, DOWN},
    {155588, DOWN},
    {155593, DOWN},
    {155598, UP},
    {155603, UP},
    {155608, UP},
    {155613, UP},
    {155618, UP},
    {-1, -1}};
struct slider_row row_3_d_u_d[] = {
    {155574, DOWN},
    {155577, DOWN},
    {155584, DOWN},
    {155587, DOWN},
    {155594, DOWN},
    {155599, UP},
    {155602, UP},
    {155609, UP},
    {155612, UP},
    {155619, UP},
    {-1, -1}};
struct slider_row row_3_e_u_d[] = {
    {155575, DOWN},
    {155576, DOWN},
    {155585, DOWN},
    {155586, DOWN},
    {155595, DOWN},
    {155600, UP},
    {155601, UP},
    {155610, UP},
    {155611, UP},
    {155620, UP},
    {-1, -1}};
struct slider_row row_4_a_u_d[] = {
    {155596, DOWN},
    {155625, UP},
    {155605, DOWN},
    {155606, DOWN},
    {155615, DOWN},
    {155616, DOWN},
    {155626, UP},
    {155635, UP},
    {155636, UP},
    {155645, UP},
    {-1, -1}};
struct slider_row row_4_b_u_d[] = {
    {155597, DOWN},
    {155604, DOWN},
    {155607, DOWN},
    {155614, DOWN},
    {155617, DOWN},
    {155624, UP},
    {155627, UP},
    {155634, UP},
    {155637, UP},
    {155644, UP},
    {-1, -1}};
struct slider_row row_4_c_u_d[] = {
    {155598, DOWN},
    {155603, DOWN},
    {155608, DOWN},
    {155613, DOWN},
    {155618, DOWN},
    {155623, UP},
    {155628, UP},
    {155633, UP},
    {155638, UP},
    {155643, UP},
    {-1, -1}};
struct slider_row row_4_d_u_d[] = {
    {155599, DOWN},
    {155602, DOWN},
    {155609, DOWN},
    {155612, DOWN},
    {155619, DOWN},
    {155622, UP},
    {155629, UP},
    {155632, UP},
    {155639, UP},
    {155642, UP},
    {-1, -1}};
struct slider_row row_4_e_u_d[] = {
    {155600, DOWN},
    {155601, DOWN},
    {155610, DOWN},
    {155611, DOWN},
    {155620, DOWN},
    {155621, UP},
    {155630, UP},
    {155631, UP},
    {155640, UP},
    {155641, UP},
    {-1, -1}};

void open_exit(struct slider_row row)
{
  REMOVE_BIT(EXITN(row.room, row.door)->exit_info, EX_CLOSED);
  REMOVE_BIT(EXITN(row.room, row.door)->exit_info, EX_LOCKED);
  // REMOVE_BIT(EXITN(row.room, row.door)->exit_info, EX_HIDDEN3);
  SET_BIT(EXITN(row.room, row.door)->exit_info, EX_PICKPROOF);
}

void close_exit(struct slider_row row)
{
  SET_BIT(EXITN(row.room, row.door)->exit_info, EX_CLOSED);
  SET_BIT(EXITN(row.room, row.door)->exit_info, EX_LOCKED);
  // SET_BIT(EXITN(row.room, row.door)->exit_info, EX_HIDDEN3);
  SET_BIT(EXITN(row.room, row.door)->exit_info, EX_PICKPROOF);
}

static void send_to_cube(const char *echo)
{
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next)
  {
    if (!d->character)
      continue;
    if (GET_ROOM_VNUM(d->character->in_room) < 155521)
      continue;
    if (GET_ROOM_VNUM(d->character->in_room) > 155641)
      continue;

    if (!AWAKE(d->character))
      continue;
    send_to_char(d->character, echo);
  }
}

void open_row(struct slider_row *row)
{
  open_msg = TRUE;
  while (row->room != -1)
    open_exit(*row++);
}

void close_row(struct slider_row *row)
{
  close_msg = TRUE;
  while (row->room != -1)
    close_exit(*row++);
}

void toggle_row(struct slider_row *row)
{
  if (IS_CLOSED(row->room, row->door))
    open_row(row);
  else
    close_row(row);
}

// How long average probability between polls..
#define TOG_DELAY 20
// Probability for a wall to toggle..  close to 1000 less likely.
#define TOG_FREQ 975

SPECIAL(cube_slider)
{
  if (cmd)
    return 0;

  if (!rand_number(0, TOG_DELAY))
    return 0;

  open_msg = FALSE;
  close_msg = FALSE;

  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_a_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_b_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_c_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_d_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_e_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_a_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_b_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_c_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_d_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_e_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_a_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_b_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_c_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_d_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_e_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_a_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_b_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_c_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_d_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_e_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_a_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_b_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_c_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_d_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_e_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_a_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_b_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_c_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_d_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_e_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_a_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_b_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_c_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_d_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_e_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_a_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_b_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_c_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_d_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_e_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_a_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_b_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_c_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_d_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_e_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_a_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_b_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_c_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_d_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_e_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_a_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_b_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_c_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_d_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_e_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_a_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_b_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_c_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_d_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_e_u_d);

  if (open_msg)
    send_to_cube("\tD\tLThe whole area rumbles loudly as a dividing wall slams shut.\tn\r\n");
  if (close_msg)
    send_to_cube("\tDEverything begins to shake and rumble as a dividing wall opens.\tn\r\n");

  return 1;
}

/*****************/
/* End Mad Drow */
/*****************/

/*****************/
/* Temple of Twisted Flesh (TTF) */

/*****************/

SPECIAL(ttf_monstrosity)
{
  struct char_data *vict;
  struct char_data *next_vict;
  int percent, prob;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return 0;

  if (!FIGHTING(ch))
    return 0;

  if (dice(1, 10) > 2)
    return 0;

  act("\tLThe tentacled monstrosity rises up in the air and sends its full mass crashing into the floor!\tn",
      FALSE, ch, 0, 0, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;

    if (!aoeOK(ch, vict, -1))
      continue;

    percent = rand_number(1, 101); /* 101% is a complete failure */
    prob = GET_LEVEL(ch) / 5;
    if (percent < prob)
    {
      change_position(vict, POS_SITTING);
      WAIT_STATE(vict, 1 * PULSE_VIOLENCE);
      act("\trThe shockwave sends you crashing to the ground!\tn", FALSE, vict, 0, 0, TO_CHAR);
      act("\trThe shockwave sends \tn$n\tr crashing to the ground!\tn", FALSE, vict, 0, 0, TO_ROOM);
    }
  }
  return TRUE;
}

SPECIAL(ttf_abomination)
{
  struct char_data *vict;
  struct char_data *next_vict;
  int percent, prob;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return 0;

  if (!FIGHTING(ch))
    return 0;

  if (dice(1, 16) > 2)
    return 0;

  act("\tLA gargantuan four-armed battle abomination lunges forward and swings one of his\r\n"
      "\tLenormous arms straight into your group!\tn",
      FALSE, ch, 0, 0, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;

    if (!aoeOK(ch, vict, -1))
      continue;

    percent = rand_number(1, 101); /* 101% is a complete failure */
    prob = GET_LEVEL(ch) / 5;
    if (percent < prob)
    {
      change_position(vict, POS_SITTING);
      WAIT_STATE(vict, 1 * PULSE_VIOLENCE);
      act("\trYou are unable to dodge the blow, and its force sends you crashing to the ground!\tn",
          FALSE, vict, 0, 0, TO_CHAR);
      act("$n \tris unable to dodge the blow, and its force sends $m crashing to the ground!\tn",
          FALSE, vict, 0, 0, TO_ROOM);
    }
  }
  return TRUE;
}

SPECIAL(ttf_rotbringer)
{
  int hp;
  struct char_data *mob;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return 0;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;
  if (!FIGHTING(ch))
    return 0;

  if (PROC_FIRED(ch))
    return FALSE;

  hp = GET_HIT(ch) * 100;
  hp /= GET_MAX_HIT(ch);
  if (hp < 40)
  {
    send_to_room(ch->in_room,
                 "\tRThe Rot Bringer realizes the tide of the battle is turning against him, and he\tn\r\n"
                 "\tRtakes a step towards the bloody basin. His face contorted in rage, he whispers\tn\r\n"
                 "\tRsomething while clawing at the air over the floating bodies. Instantly, the red\tn\r\n"
                 "\tRliquid starts swirling as the cadavers join together, forming a massive mound of\tn\r\n"
                 "\tRmeat! A massive ball of flesh rises out of the basin, and follows its new master!\tn\r\n");

    mob = read_mobile(145193, VIRTUAL);
    char_to_room(mob, ch->in_room);
    add_follower(mob, ch);
    PROC_FIRED(ch) = TRUE;

    return TRUE;
  }
  return FALSE;
}

int ttf_path[] = {
    145185, 145184, 145183, 145184, 145186, 145187, 145186, 145188, 145189, 145188,
    145186, 145187, 145186, 145184, 145185, -1};

SPECIAL(ttf_patrol)
{
  int dir = -1;
  // int next = 0;

  if (!ch)
    return 0;
  if (FIGHTING(ch))
    return 0;

  if (cmd)
    return 0;

  if (PATH_INDEX(ch) > 16 || PATH_INDEX(ch) < 0)
    PATH_INDEX(ch) = 0;

  // 8 second delay...
  if (PATH_DELAY(ch) > 0)
  {
    PATH_DELAY(ch)
    --;
    return 0;
  }
  PATH_DELAY(ch) = 8;

  PATH_INDEX(ch)
  ++;

  if (ttf_path[PATH_INDEX(ch)] == -1)
    PATH_INDEX(ch) = 0;

  dir = find_first_step(ch->in_room, real_room(ttf_path[PATH_INDEX(ch)]));
  if (dir >= 0)
    perform_move(ch, dir, 1);
  return 1;
}

/*****************/
/* End Temple of Twisted Flesh (TTF) */
/*****************/

/* put new zone procs here */
