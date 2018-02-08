/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\                                                             
/  Luminari Psionics System, Inspired by Outcast Psionics                                                           
/  Created By: Altherog, ported by Zusuk                                                           
\                                                             
/  using psionics.h as the header file currently                                                           
\         todo: actually make a pathfinder system                                                   
/                                                                                                                                                                                       
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

#include <zconf.h>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "psionics.h"
#include "feats.h"
/* spells.h included in header file */

//#define PSI_CLASS_ENABLED

/* GLOBALS */
ush_int indexToPspTable[MAX_SKILLS];

/* UTILITY FUNCTIONS */

/* PRIMARY FUNCTIONS */



/* EOF */






/* Initialize the PSP Cost Table during boot. It basicaly allows to move the
 * skills around without need to update some other lookup tables..
 */
void bootInitializePSPTable(void) {
  int i = 0;

  bzero(indexToPspTable, sizeof (indexToPspTable));

  while (pspTable[i].skill != SPELL_RESERVED_DBC)
    indexToPspTable[pspTable[i].skill] = i++;
}
#ifdef PSI_CLASS_ENABLED

/* this function replaces all psi damage calculations (formerly handled
   within each skill function) by assigning an offensive level and determining
   damage in the FindSpellDamage function.  --DMB */
int psiDamage(struct char_data *ch, P_char vict, int dam, int skill) {
  int adj_skill, off_level;

  switch (skill) {
    case PSIONIC_MINDBLAST:
      off_level = 11;
      break;
    case PSIONIC_DETONATE:
      off_level = 13;
      break;
    case PSIONIC_PROJECT_FORCE:
      off_level = 15;
      break;
    case PSIONIC_DEATH_FIELD:
      off_level = 16;
      break;
    case PSIONIC_ULTRABLAST:
      off_level = 20;
      break;
  }

  adj_skill = pindex2Skill[skill];
  dam = FindSpellDamage(ch, vict, off_level, skill);

  if (skill != PSIONIC_DEATH_FIELD) {
    if (NewSaves(vict, SAVING_BREATH, 1))
      dam >>= 1;
  }

  if (IS_PC(ch) && IS_PC(vict))
    dam = adjustSkillDamage(dam, adj_skill);

  return dam;
}

bool canDrain(struct char_data *ch, P_char victim) {
  if (IS_AFFECTED(victim, AFF_CHARM) && (victim->following == ch))
    return TRUE;

  if (IS_PC(victim) && IS_PC(ch) && !IN_ACHERON(victim)) {
    send_to_char("Player killing isn't allowed!\n", ch);
    return FALSE;
  }

  if (victim == ch) {
    send_to_char("Aren't we funny today...\n", ch);
    return FALSE;
  }
  if (GET_STAT(victim) == STAT_DEAD) {
    send_to_char("What?  Dead isn't good enough?  Leave that corpse alone!\n", ch);
    return FALSE;
  }
  if (!AWAKE(ch)) {
    send_to_char("You can't do that if you're not awake!\n", ch);
    return FALSE;
  }
  if (affected_by_spell(ch, SONG_PEACE)) {
    send_to_char("You feel way too peaceful to consider doing anything offensive!\n", ch);
    return FALSE;
  }
  if (CHAR_IN_SAFE_ZONE(ch)) {
    send_to_char("You feel ashamed trying to disrupt the tranquility of this place.\n", ch);
    return FALSE;
  }
  if (IS_RIDING(ch)) {
    send_to_char("While mounted? I don't think so...\n", ch);
    return FALSE;
  }
  if (IS_CSET(world[ch->in_room].room_flags, SINGLE_FILE) && !AdjacentInRoom(ch, victim)) {
    act("$N seems to be just a BIT out of reach.", FALSE, ch, 0, victim, TO_CHAR);
    return FALSE;
  }
  if (!CAN_SEE(ch, victim)) {
    send_to_char("Um.. you don't see any such target here?\n", ch);
    return FALSE;
  }
  if (nokill(ch, victim))
    return FALSE;

  if (CHAR_IN_SAFE_ZONE(ch)) {
    send_to_char("You feel ashamed trying to disrupt the tranquility of this place.\n", ch);
    return FALSE;
  }
  if (IS_AFFECTED(ch, AFF_STUNNED)) {
    send_to_char("You're too stunned to contemplate that!\n", ch);
    return FALSE;
  }
  if (IS_AFFECTED(ch, AFF_KNOCKED_OUT)) {
    send_to_char("You can't do much of anything while knocked out!\n", ch);
    return FALSE;
  }
  if (IS_AFFECTED(ch, AFF_MINOR_PARALYSIS) || IS_AFFECTED(ch, AFF_MAJOR_PARALYSIS)) {
    send_to_char("It's tough to do anything while paralyzed.\n", ch);
    return FALSE;
  }
  if (IS_AFFECTED(victim, AFF_CHARM) && (victim->following != ch))
    return FALSE;

  return TRUE;
}

void do_drain(struct char_data *ch, char *arg, int cmd) {
  P_char victim;
  int drain_delay;

  if (!ch) {
    log("assert: bogus params in do_drain()");
    dump_core();
  }

  if (GET_RACE(ch) != RACE_ILLITHID) {
    send_to_char("You have absolutely no idea how to go about doing that.\n", ch);
    return;
  }

  if (!*arg) {
    send_to_char("So just WHO would you like to have for dinner again?\n", ch);
    return;
  }
  if (!(victim = get_char_room_vis(ch, arg))) {
    send_to_char("Unfortunately your victim does not happen to be here.\n", ch);
    return;
  }

  /* use the normal candofightmove since draining is a physical not a mental
   * skill.. ie cant do it while paralyzed or out of reach in a single file
   * room..                                                                  */
  if (!canDrain(ch, victim))
    return;

  if (IS_AFFECTED(ch, AFF_BRAINDRAIN)) {
    send_to_char("&+LError in do_brain() synchronization. Report to a God.\n", ch);
    wizlog(51, "ERROR: do_drain() called with AFF_BRAINDRAIN on for %s and %s", GET_NAME(ch), GET_NAME(victim));
    return;
  }

  switch (GET_RACE(victim)) {
    case RACE_ILLITHID:
      /* Nasty surprise while trying to drain fellow illithids.. Take away all psp */
      send_to_char("&+LDrain your own brethren?!\n", ch);
      send_to_char("&+LYou feel a wave of pain roll over you.\n", ch);
      act("&+L$n shivers and staggers back as $s his tentacles touch&N $N's &+Lhead.", TRUE, ch, 0, victim, TO_ROOM);
      if (GET_PSP(ch) > 0 && !IS_TRUSTED(ch)) {
        GET_PSP(ch) = 0;
        StartRegen(ch, EVENT_PSP_REGEN);
      }
      return;
    case RACE_SPIRIT:
    case RACE_F_ELEMENTAL:
    case RACE_W_ELEMENTAL:
    case RACE_A_ELEMENTAL:
    case RACE_E_ELEMENTAL:
    case RACE_GHOST:
    case RACE_INSECT:
    case RACE_TREE:
    case RACE_PARASITE:
    case RACE_POSSESSED:
    case RACE_HIGH_UNDEAD:
    case RACE_UNDEAD:
      send_to_char("&+LDraining that creature would be rather impossible.\n", ch);
      act("&+L$n futilely tries to drain&N $N.", TRUE, ch, 0, victim, TO_ROOM);
      return;
    default:
      break;
  }

  if ((GET_STAT(victim) >= STAT_SLEEPING) && (!IS_AFFECTED(victim, AFF_CHARM))) {
    send_to_char("You have to subdue your victim first!\n", ch);
    return;
  }

  SET_CBIT(ch->specials.affects, AFF_BRAINDRAIN);
  drain_delay = MAX(4, (GET_LEVEL(victim) / 2));
  AddEvent(EVENT_BRAIN_DRAINING, drain_delay, TRUE, ch, victim);
  act("&+LYou kneel down and grasp&N $N's &+Lhead in your tentacles.", TRUE, ch, 0, victim, TO_CHAR);
  act("&+LYou start to feed on $S energy!", TRUE, ch, 0, victim, TO_CHAR);
  act("&+L$n kneels down and grasps&N $N's &+Lhead with $s tentacles.", TRUE, ch, 0, victim, TO_ROOM);
  send_to_char("&+LA flash of pain grips your brain, as something starts to drain your energies.&N\n", victim);
  CharWait(ch, drain_delay + 12); /* lag the sucker so it wont move away */
  return;
}

void illithid_feeding(struct char_data *ch, P_char victim) {
  if (!(victim && ch)) {
    log("assert: bogus params in illithid_feeding()");
    dump_core();
  }

  if (ch->in_room == NOWHERE)
    return;

  if (!IS_AFFECTED(ch, AFF_BRAINDRAIN)) {
    log("assert: bogus illithid_feeding call. No AFF_BRAINDRAIN.");
    dump_core();
  }

  REMOVE_CBIT(ch->specials.affects, AFF_BRAINDRAIN);

  if (ch->in_room != victim->in_room) {
    send_to_char("Your victim does not seem to be here anymore.\n", ch);
    send_to_char("That's it for feeding this time!\n", ch);
    act("$n looks extremely confused as he aborts $s feeding.", TRUE, ch, 0, victim, TO_ROOM);
    return;
  }

  act("&+mThe feast comes to an end as you loosen your tentacles' grip.", TRUE, ch, 0, victim, TO_CHAR);
  act("&+m$n loosens $s grip on&N $N's &+mhead and stands up.", TRUE, ch, 0, victim, TO_ROOM);
  send_to_char("&+mAll of your energy is slowly slurped from your brain and you "
          "feel yourself falling into the cold sleep of death...&N\n", victim);
  GET_PSP(ch) += (GET_C_INT(victim) * 8) + GET_C_WIS(victim);

  if (GET_PSP(ch) > GET_MAX_PSP(ch))
    GET_PSP(ch) = GET_MAX_PSP(ch);

  if (GET_COND(ch, FULL) < 100)
    GET_COND(ch, FULL) += 25;
  if (GET_COND(ch, THIRST) < 100)
    GET_COND(ch, THIRST) += 25;
  SuddenDeath(victim, ch, "brain drain");
  return;
}

/**********************   CLAIRSENTIENCE SKILLS   **************************/
void do_aurasight(struct char_data *ch, char *arg, int cmd) {
  P_char victim;
  int skl_lvl, align, psp;

  if (!ch) {
    log("assert: bogus param in do_aurasight");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_AURASIGHT);

  if (psp == 0) {
    send_to_char("You have absolutely no idea how to go about doing that.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_AURASIGHT);
  victim = ParseTarget(ch, arg);

  if (!victim) {
    send_to_char("Who's Aura would you like to see again?\n", ch);
    return;
  }

  if (ch == victim) {
    send_to_char("Nice try but you cannot see your own aura.\n", ch);
    return;
  }

  align = GET_ALIGNMENT(victim);

  if (align > 800)
    sprintf(buf, "$N&+Y has a blinding golden aura around $S body.\n");
  else if (align > 350)
    sprintf(buf, "$N&+Y has a well defined golden aura around $S body.\n");
  else if (align > 100)
    sprintf(buf, "$N&+g has a bearly perceptible trace of golden aura around $S body.&N\n");
  else if (align > -100)
    sprintf(buf, "$N&+g has a shimmering, multi-coloured aura around $M.&N\n");
  else if (align > -350)
    sprintf(buf, "$N&+r has a noticable redish aura around $M.&N\n");
  else if (align > -700)
    sprintf(buf, "$N&+r has a very strong red aura around $M.&N\n");
  else if (align >= -1001)
    sprintf(buf, "$N&+r has a blinding red aura around $M.&N\n");
  else sprintf(buf, "$N &=rlradiates incredible amounts of evil energy.\nYou start feeling weak just by looking at $M.&N&n\n");

  if (affected_by_spell(victim, SPELL_CURSE))
    sprintf(buf, "$N&N&+r seems to be affected by some sort of a curse.\n");

  if (IS_NPC(victim)) {
    if (IS_CSET(victim->only.npc->npcact, ACT_AGGRESSIVE_EVIL))
      sprintf(buf, "$N&N&+r hates evil.\n");
    if (IS_CSET(victim->only.npc->npcact, ACT_AGGRESSIVE_GOOD))
      sprintf(buf, "$N&N&+y hates good.\n");
    if (IS_CSET(victim->only.npc->npcact, ACT_AGGRESSIVE_NEUTRAL))
      sprintf(buf, "$N&N&+r hates neutral.\n");
    if (IS_CSET(victim->only.npc->npcact, ACT_AGG_RACEEVIL))
      sprintf(buf, "$N&N&+r hates evil races.\n");
    if (IS_CSET(victim->only.npc->npcact, ACT_AGG_RACEGOOD))
      sprintf(buf, "$N&N&+r hates good races.\n");
    if (IS_CSET(victim->only.npc->npcact, ACT_AGG_OUTCAST))
      sprintf(buf, "$N&N&+r hates outcasts.\n");
  }

  act(buf, TRUE, ch, 0, victim, TO_CHAR);
  incSkillSubPsp(ch, PSIONIC_AURASIGHT, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, victim, arg, PSIONIC_AURASIGHT);

  return;
}

void do_combatmind(struct char_data *ch, char *arg, int cmd) {
  P_char victim = 0;
  int skl_lvl, psp;
  struct affected_type af;

  if (!ch) {
    log("assert: bogus param in do_combatmind");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_COMBATMIND);

  if (psp == 0) {
    send_to_char("If you only had an idea how to go about it..\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_COMBATMIND);

  if (arg) {
    victim = ParseTarget(ch, arg);
    if (!victim) {
      send_to_char("Which slave was that again?\n", ch);
      return;
    }
    if (IS_PC(victim)) {
      send_to_char("You can only project it on your slaves.\n", ch);
      return;
    }
    if (victim->following != ch) {
      send_to_char("You can only project that onto your followers.\n", ch);
      return;
    }
  }

  if (affected_by_spell(victim, PSIONIC_COMBATMIND)) {
    send_to_char("The projection does not seem to make any difference!\n", ch);
    return;
  }

  bzero(&af, sizeof (af));
  af.type = PSIONIC_COMBATMIND;
  af.duration = MAX(5, GET_SKILL(ch, PSIONIC_COMBATMIND) / 10);
  af.modifier = GET_LEVEL(ch) / 6;
  CLEAR_CBITS(af.sets_affs, AFF_BYTES);
  affect_to_char(victim, &af);
  af.location = APPLY_HITROLL;
  af.modifier = GET_LEVEL(ch) / 8;
  af.location = APPLY_DAMROLL;
  affect_to_char(victim, &af);
  af.modifier = -(GET_LEVEL(ch) / number(1, 2) - 10);
  af.location = APPLY_AC;
  affect_to_char(victim, &af);

  act("&+cYou are suddenly filled with knowledge of battle tactics!", TRUE, victim, 0, 0, TO_CHAR);
  act("$n &N&+cbecomes more alert, evaluating possible targets.", TRUE, victim, 0, 0, TO_ROOM);
  act("$N &N&+cbecomes more alert as a result of your projection", TRUE, ch, 0, victim, TO_CHAR);

  incSkillSubPsp(ch, PSIONIC_COMBATMIND, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, victim, arg, PSIONIC_COMBATMIND);

  return;
}

void do_vision(struct char_data *ch, char *arg, int cmd) {
  P_char victim = 0;
  int skl_lvl;
  struct affected_type af;

  if (!ch) {
    logit("assert: bogus param in do_vision");
    dump_core();
  }

  if (!canUsePsionicSkill(ch, PSIONIC_VISION)) {
    send_to_char("You have no idea how to enhance your senses.\n", ch);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_VISION);

  if (arg) {
    victim = ParseTarget(ch, arg);
    if (ch != victim) {
      send_to_char("You can only project that onto yourself.\n", ch);
      return;
    }
  }

  if (affected_by_spell(ch, PSIONIC_VISION)) {
    send_to_char("There's only that much you can see!\n", ch);
    return;
  }

  bzero(&af, sizeof (af));
  af.type = PSIONIC_VISION;
  af.duration = MAX(5, GET_SKILL(ch, PSIONIC_VISION) / 5);
  SET_CBIT(af.sets_affs, AFF_DETECT_MAGIC);
  SET_CBIT(af.sets_affs, AFF_SENSE_LIFE);
  SET_CBIT(af.sets_affs, AFF_DETECT_INVISIBLE);
  affect_to_char(ch, &af);


  act("&+cYou are suddenly filled with knowledge of battle tactics!", TRUE, ch, 0, victim, TO_CHAR);
  act("&+cA glimmer of understanding crosses $N's face.", TRUE, ch, 0, victim, TO_ROOM);

  incSkillSubPsp(ch, PSIONIC_VISION, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, victim, arg, PSIONIC_VISION);

  return;
}

int sense_aggros_in_room(struct char_data *ch, int room) {
  P_char tmp_vict = NULL, temp = NULL;
  int maxlevel = 0;

  for (tmp_vict = world[room].people; tmp_vict; tmp_vict = temp) {
    temp = tmp_vict->next_in_room;
    if (IS_NPC(tmp_vict) && is_aggr_to_remote(tmp_vict, ch))
      if (GET_LEVEL(tmp_vict) > maxlevel)
        maxlevel = GET_LEVEL(tmp_vict);
  }

  return maxlevel;

}

/* Two char arrays.. _ms for the messages send to PC's with mediocre
 * sense danger skill.. _hs for the once with high skill proficiency      */
const char *sense_danger_ms[] = {
  "You sense a mind full of spite nearby.\n",
  "You sense something very malignant nearby.\n",
  "You sense a vicious and calculating presence nearby.\n",
  "You sense a presence filled with devilish intentions towards your kind nearby.\n",
  "You are nearly overwhelmed by the sense of diabolical hatred directed\ntowards your kind. It seems to be coming from somewhere close by.\n",
  "\n"
};

const char *sense_danger_hs[] = {
  "You sense a mind filled with spitful thoughts ",
  "You sense a mind full malignant thoughts ",
  "You sense something vicious and calculating ",
  "You sense a presence filled with devilish intentions towards your kind ",
  "You are nearly overwhelmed by the sense of diabolical hatred directed\ntowards your kind. It seems to be located ",
  "\n"
};

const char *sense_danger_dir[] = {
  "to the north.\n",
  "to the east.\n",
  "to the south.\n",
  "to the west.\n",
  "directly above you.\n",
  "directly below you.\n",
  "\n"
};

void do_sense_danger(struct char_data *ch, char *arg, int cmd) {
  int skl_lvl, lvl_diff, msgindex, dir, aggro_lvls[7], exitflag = TRUE, psp;
  struct room_direction_data *temp = NULL;

  if (!ch) {
    log("assert: bogus param in do_sense_danger");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_SENSE_DANGER);

  if (psp == 0) {
    send_to_char("You don't know how to sense remote danger.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_SENSE_DANGER);

  bzero(aggro_lvls, sizeof (aggro_lvls));

  /* Added a patch for exits leading to NOWHERE.  --DMB 10/25/98 */
  /* condensed 6 seperate checks to a loop,
     and eliminated return and error msg on NOWHERE rooms since it wasn't needed -Azuth */
  for (dir = NORTH; dir < NUMB_EXITS; dir++) {
    temp = world[ch->in_room].dir_option[dir];
    if (temp && temp->to_room && temp->to_room != NOWHERE &&
            !IS_SET(temp->exit_info, EX_SECRET) &&
            !IS_SET(temp->exit_info, EX_BLOCKED))
      aggro_lvls[dir] = sense_aggros_in_room(ch, real_room(world[temp->to_room].number));
  }

  buf[0] = 0;

  /* Cause shit at this point.. If the level difference between the PC and the
   * aggro mind it senses is larger then 20, make the PC roll a paralysis check
   * and force flee if it hits.. nasty stuff in high level zones */
  for (dir = 0; dir < 7; dir++) {
    if (aggro_lvls[dir])
      exitflag = FALSE;
    lvl_diff = aggro_lvls[dir] - GET_LEVEL(ch);
    if (lvl_diff > 20) {
      if (!NewSaves(ch, SAVING_PARA, -2) && !IS_TRUSTED(ch)) {
        send_to_char("The viciousness of the mind you have just sensed makes you panic in utter terror.\n", ch);
        incSkillSubPsp(ch, PSIONIC_SENSE_DANGER, psp, PSIONIC_GAIN_ON);
        psiSkillUsageLogging(ch, 0, arg, PSIONIC_SENSE_DANGER);
        do_flee(ch, 0, 1);
        return;
      }
    }
  }

  /* Bail out since there was no aggros detected around the char */

  if (exitflag) {
    send_to_char("You don't seem to detect anything angry at you around here.\n", ch);
    incSkillSubPsp(ch, PSIONIC_SENSE_DANGER, psp, PSIONIC_GAIN_ON);
    psiSkillUsageLogging(ch, 0, arg, PSIONIC_SENSE_DANGER);
    return;
  }

  /* go thru 3 levels of proficiency checks to determine the amount of message
   * detail the PC gets.. */
  if (skl_lvl < 40) {
    for (dir = 0; dir < 6; dir++) {
      if (aggro_lvls[dir]) {
        sprintf(buf, "You sense a mind filled with hatred towards your kind nearby.\n");
        break;
        /* Warn the PC about just one threat */
      }
    }
  } else {
    if (skl_lvl < 70) {
      for (dir = 0; dir < 6; dir++)
        if (aggro_lvls[dir]) {
          lvl_diff = aggro_lvls[dir] - GET_LEVEL(ch);
          if (lvl_diff < -5) msgindex = 0;
          else
            if (lvl_diff < 5) msgindex = 1;
          else
            if (lvl_diff < 15) msgindex = 2;
          else
            if (lvl_diff < 25) msgindex = 3;
          else msgindex = 4;
          /* Warn the PC about just one threat */
          strcat(buf, sense_danger_ms[msgindex]);
        }

    } else {
      /* sense danger skill over 70%.. Let the PC know the direction as well */
      for (dir = 0; dir <= 6; dir++) {
        if (aggro_lvls[dir]) {
          lvl_diff = aggro_lvls[dir] - GET_LEVEL(ch);
          if (lvl_diff < -5) msgindex = 0;
          else
            if (lvl_diff < 5) msgindex = 1;
          else
            if (lvl_diff < 15) msgindex = 2;
          else
            if (lvl_diff < 25) msgindex = 3;
          else msgindex = 4;
          strcat(buf, sense_danger_hs[msgindex]);
          strcat(buf, sense_danger_dir[dir]);
        }
      }
    }
  }

  send_to_char(buf, ch);

  incSkillSubPsp(ch, PSIONIC_SENSE_DANGER, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, 0, arg, PSIONIC_SENSE_DANGER);

  return;
}

void do_mindblast(struct char_data *ch, char *arg, int cmd) {
  P_char victim;
  int skl_lvl, percent, dam, psp, resist = 0, stun_time = 0, will = 0;

  if (!ch) {
    log("assert: bogus param in do_mindblast");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_MINDBLAST);

  if (psp == 0) {
    send_to_char("You don't know how to mind blast.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_MINDBLAST);

  victim = ParseTarget(ch, arg);

  if (!victim) {
    send_to_char("Mind blast who?\n", ch);
    return;
  }

  /* Check for single file room.  If PC isn't next to victim, don't let them
     do it. DMB 10/11/98 */
  if (IS_CSET(world[ch->in_room].room_flags, SINGLE_FILE) && !AdjacentInRoom(ch, victim)) {
    send_to_char("You can't get a clear line of sight in these cramped quarters.\n", ch);
    return;
  }

  if (!CanPsiDoFightMove(ch, victim))
    return;

  skl_lvl = GET_SKILL(ch, PSIONIC_MINDBLAST);

  if (IS_NPC(ch))
    skl_lvl = (GET_LEVEL(ch) * 1.5);
  dam = dice(MIN(GET_LEVEL(ch), 33), 6) + (skl_lvl >> 2);
  dam *= DAMFACTOR;

  act("&+BYou can sense the growing pain in&N $N's &+Bmind.&N", TRUE, ch, 0, victim, TO_CHAR);
  act("&+BA wave of intense pain suddenly dims your mind!", TRUE, ch, 0, victim, TO_VICT);
  act("$N &+Bseems to be in a great deal of pain all of a sudden.", TRUE, ch, 0, victim, TO_NOTVICT);

  // borrowed from DoesPsiResist
  /* if victim does not have above 100 adjusted INT, no chance to resist */
  if (GET_C_INT(victim) > 100) {
    resist = (GET_C_INT(victim) - 80);
    if (GET_C_POW(ch) > 110)
      will = (GET_C_POW(ch) - 110);
    resist -= will;
    if (resist > 0)
      resist += (GET_LEVEL(ch) - GET_LEVEL(victim));
  }

  if (resist < 0)
    resist = 0;

  percent = number(1, 99);

  if (!resist && ((skl_lvl - percent) > 60) && (GET_RACE(victim) !=
          RACE_DRAGON)) {
    stun_time = number(1, 3);
    Stun(victim, stun_time * PULSE_VIOLENCE);
  }

  dam = psiDamage(ch, victim, dam, PSIONIC_MINDBLAST);
  damage(ch, victim, dam, PSIONIC_MINDBLAST);

  incSkillSubPsp(ch, PSIONIC_MINDBLAST, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, victim, arg, PSIONIC_MINDBLAST);

  return;
}

/**********************   PSYCHOKINETIC SKILLS   **************************/
void do_detonate(struct char_data *ch, char *arg, int cmd) {
  const int dam_detonate[] = {0, 0, 0, 0, 0, 0, 0, 0, 20, 20,
    25, 30, 35, 35, 40, 40, 45, 45, 50, 50,
    55, 55, 60, 60, 65, 65, 70, 75, 80, 85,
    95, 100, 105, 110, 125, 130, 135, 140, 145, 150,
    160, 170, 180, 190, 200, 210, 220, 230, 240, 250, 260};
  int dam, level, skl_lvl = 0, psp, can_det = 0;
  P_char victim;

  if (!ch) {
    log("assert: bogus parm in do_detonate");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_DETONATE);

  if (psp == 0) {
    send_to_char("You don't know how to detonate.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_DETONATE);
  if (IS_NPC(ch))
    skl_lvl = (GET_LEVEL(ch) * (3 / 2));

  victim = ParseTarget(ch, arg);

  if (!victim) {
    send_to_char("Detonate who what?\n", ch);
    return;
  }

  /* Check for single file room.  If PC isn't next to victim, don't let them
     do it. DMB 10/11/98 */

  if (IS_CSET(world[ch->in_room].room_flags, SINGLE_FILE) && !AdjacentInRoom(ch, victim)) {
    send_to_char("You can't get a clear line of sight in these cramped quarters.\n", ch);
    return;
  }

  if (!CanPsiDoFightMove(ch, victim))
    return;

  /* Has effect only on the following races as per ad&d detonate */

  switch (GET_RACE(victim)) {
    case RACE_POSSESSED:
    case RACE_HIGH_UNDEAD:
    case RACE_UNDEAD:
    case RACE_GOLEM:
    case RACE_TREE:
    case RACE_PARASITE:
    case RACE_LICH:
    case RACE_VAMPIRE:
      can_det = 1;
      break;
    default:
      can_det = 0;
  }

  if (GET_CLASS(victim) == CLASS_LICH)
    can_det = 1;

  if (can_det != 1) {
    send_to_char("Your projection does not seem to have any effect on this creature.\n", ch);
    incSkillSubPsp(ch, PSIONIC_DETONATE, psp, PSIONIC_GAIN_ON);
    psiSkillUsageLogging(ch, victim, arg, PSIONIC_DETONATE);
    return;
  }

  if (!CanDoFightMove(ch, victim, 0))
    return;

  level = BOUNDED(0, GET_LEVEL(ch), 50);
  dam = MAX(0, dam_detonate[level] + (number(1, 50) - 25));

  if (GET_LEVEL(ch) > 30)
    dam += skl_lvl;

  if (!number(0, 20))
    dam <<= 1;

  if (saves_spell(victim, SAVING_BREATH))
    dam >>= 1;

  act("Parts of $N's&N matter explode outward as your projection hits it.", FALSE, ch, 0, victim, TO_CHAR);
  act("You are overwhelmed by a great pain as $n&N does something with your body!", FALSE, ch, 0, victim, TO_VICT);
  act("A fine mist of particles explodes outward from $N's&N body!", FALSE, ch, 0, victim, TO_NOTVICT);

  dam = psiDamage(ch, victim, dam, PSIONIC_DETONATE);
  damage(ch, victim, dam, PSIONIC_DETONATE);

  incSkillSubPsp(ch, PSIONIC_DETONATE, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, victim, arg, PSIONIC_DETONATE);

  return;
}

int do_project_force(struct char_data *ch, char *arg, int cmd)
/* Added True/False return to do_project_force */
/* Returns TRUE if an attempt to project was made */
/* FALSE if no attempt made */
/* Iyachtu 8/24/01 */ {
  int dam, skl_lvl = 0, psp;
  P_char victim;
  const int dam_project[] ={0, 0, 0, 0, 0, 0, 0, 0, 40, 40,
    42, 42, 45, 47, 50, 50, 50, 50, 55, 55,
    55, 55, 60, 60, 65, 65, 70, 75, 80, 85,
    95, 100, 105, 110, 125, 130, 135, 140, 145, 150,
    160, 170, 180, 190, 200, 210, 220, 230, 240, 250, 260};

  if (!ch) {
    log("assert: bogus parm in do_project_force");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_PROJECT_FORCE);

  if (psp == 0) {
    send_to_char("You don't know how to project force.\n", ch);
    return FALSE;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return FALSE;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_PROJECT_FORCE);
  if (IS_NPC(ch))
    skl_lvl = (GET_LEVEL(ch) * (3 / 2));

  victim = ParseTarget(ch, arg);

  if (!victim) {
    send_to_char("Project Force onto who?\n", ch);
    return FALSE;
  }

  /* Check for single file room.  If PC isn't next to victim, don't let them
     do it. DMB 10/11/98 */
  if (IS_CSET(world[ch->in_room].room_flags, SINGLE_FILE) && !AdjacentInRoom(ch, victim)) {
    send_to_char("You can't get a clear line of sight in these cramped quarters.\n", ch);
    return FALSE;
  }

  if (!CanPsiDoFightMove(ch, victim))
    return FALSE;

  dam = MAX(0, dam_project[BOUNDED(0, GET_LEVEL(ch), 50)] + skl_lvl);
  dam += number(-13, 13); /* randomize a bit */

  /* Moved damage to this line, so following works - Iyachtu */
  dam = psiDamage(ch, victim, dam, PSIONIC_PROJECT_FORCE);

  if ((number(0, 850 + skl_lvl) == 666) && (skl_lvl < 70)) {
    if (!saves_spell(ch, SAVING_BREATH) && (would_die(ch, dam)) && IS_PC(ch)) {
      send_to_char("&+rA terribly strong force front smashes into you as your projection missfires!", ch);
      SuddenDeath(ch, ch, "miss-fired project force <rare>");
      return TRUE;
    } else {
      send_to_char("&+rYour projection misfires badly but you are able to absorb most of the force front.\n", ch);
      GET_HIT(ch) -= dam;
      incSkillSubPsp(ch, PSIONIC_PROJECT_FORCE, psp, PSIONIC_GAIN_ON);
      psiSkillUsageLogging(ch, victim, arg, PSIONIC_PROJECT_FORCE);
      return TRUE;
    }
  }

  if (!saves_spell(victim, SAVING_BREATH))
    dam >>= 1;

  if (!IMMATERIAL(victim)) {
    act("You project a strong force front that hits $N&N dead on.", FALSE, ch, 0, victim, TO_CHAR);
    act("You stumble under the awesome force front emanating from $n's direction.", FALSE, ch, 0, victim, TO_VICT);
    act("The air between $n and $N&N is momentarily distorted.", FALSE, ch, 0, victim, TO_NOTVICT);
    if (IS_NPC(ch))
      skl_lvl = (GET_LEVEL(ch) + 15);
    if (!number(0, MAX(2, (30 - (skl_lvl >> 2)))) && (GET_RACE(victim) != RACE_DRAGON) &&
            (((GET_WEIGHT(ch) * (skl_lvl / 20)) > GET_WEIGHT(victim)) || (skl_lvl > 94))) {
      act("$N is sent sprawling all of a sudden.", FALSE, ch, 0, victim, TO_NOTVICT);
      act("It sends you sprawling.", FALSE, ch, 0, victim, TO_VICT);
      send_to_char("Your victim staggers and is sent sprawling!\n", ch);
      CharWait(victim, PULSE_VIOLENCE * (1 + number(0, 3)));
      SET_POS(victim, POS_PRONE + GET_STAT(victim));
    } else {
      act("$N&N stumbles but somehow manages to absorb the brunt of the assault.", FALSE, ch, 0, victim, TO_NOTVICT);
    }

    damage(ch, victim, dam, PSIONIC_PROJECT_FORCE);
    incSkillSubPsp(ch, PSIONIC_PROJECT_FORCE, psp, PSIONIC_GAIN_ON);
    psiSkillUsageLogging(ch, victim, arg, PSIONIC_PROJECT_FORCE);

  } else {
    send_to_char("Your projection has no visible effect.\n", ch);
    incSkillSubPsp(ch, PSIONIC_PROJECT_FORCE, psp, PSIONIC_GAIN_ON);
  }
  return TRUE;
}

/************************* PSYCHOMETABOLISM *******************************/
void do_death_field(struct char_data *ch, char *arg, int cmd) {
  P_char vict = NULL, temp = NULL;
  int skl_lvl, dam, psp, num_mobs;
  int landed = 0, attempted = 0;

  if (!ch) {
    log("assert: bogus parm in do_death_field");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_DEATH_FIELD);

  if (psp == 0) {
    send_to_char("You don't know how to create a death field.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_DEATH_FIELD);
  if (IS_NPC(ch))
    skl_lvl = (GET_LEVEL(ch) * (3 / 2));

  if (CHAR_IN_SAFE_ZONE(ch)) {
    send_to_char("You feel ashamed trying to disrupt the tranquility of this place.\n", ch);
    return;
  }

  if ((GET_ALIGNMENT(ch) >= -350) && !IS_TRUSTED(ch)) {
    send_to_char("You are not evil enough to do that!\n", ch);
    return;
  }

  if ((IS_DARK(ch->in_room) || IS_CSET(world[ch->in_room].room_flags, MAGIC_DARK))
          && !CAN_SEE(ch, ch)) {
    send_to_char("You can't even see your nose in front of your face!\n", ch);
    return;
  }


  act("&+LThe air in the immediate area darkens abruptly!&N", FALSE, ch, 0, 0, TO_ROOM);
  act("&+LThe air in the immediate area darkens abruptly as a result of your projection!&N", FALSE, ch, 0, 0, TO_CHAR);

  num_mobs = area_valid_targets(ch);

  for (vict = world[ch->in_room].people; vict; vict = temp) {
    temp = vict->next_in_room;
    if (AreaAffectCheck(ch, vict)) {

      dam = dice(((GET_LEVEL(ch) >> 2) + (skl_lvl >> 2)), 7);

      attempted++;
      dam = psiDamage(ch, vict, dam, PSIONIC_DEATH_FIELD);
      if (AreaSave(ch, vict, &dam, num_mobs) == AVOID_AREA)
        continue;
      landed++;
      if (saves_spell(vict, SAVING_PARA))
        dam >>= 1;
      damage(ch, vict, dam, PSIONIC_DEATH_FIELD);
    }
  }

  area_message(ch, landed, attempted - landed, "death field");
  incSkillSubPsp(ch, PSIONIC_DEATH_FIELD, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, 0, arg, PSIONIC_DEATH_FIELD);
  return;
}

void do_adrenalize(struct char_data *ch, char *arg, int cmd) {
  struct affected_type af;
  P_char victim;
  int psp, skl_lvl;

  if (!ch) {
    log("assert: bogus param in do_adrenalize");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_ADRENALIZE);

  if (psp == 0) {
    send_to_char("You don't know how to increase your adrenal level.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  victim = ParseTarget(ch, arg);

  if (victim != ch) {
    send_to_char("You can only perform that manipulation on your own body.\n", ch);
    return;
  }


  if (affected_by_spell(victim, PSIONIC_ADRENALIZE)) {
    send_to_char("&+rIncreasing your adrenal levels any further would be terminal!\n", victim);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_ADRENALIZE);
  if (IS_NPC(ch))
    skl_lvl = (GET_LEVEL(ch) * (3 / 2));

  bzero(&af, sizeof (af));
  af.duration = MAX(3, (skl_lvl >> 2));
  af.modifier = MAX(3, skl_lvl / 10);
  af.type = PSIONIC_ADRENALIZE;
  af.location = APPLY_HITROLL;
  CLEAR_CBITS(af.sets_affs, AFF_BYTES);
  affect_to_char(victim, &af);
  af.location = APPLY_DAMROLL;
  affect_to_char(victim, &af);
  if (IS_PC(ch)) {
    if (!number(0, skl_lvl)) {
      send_to_char("&+rYou are overwhelmed by a mad rush of adrenaline!\n", victim);
      send_to_char("&+rYour manipulation takes a toll on your health.\n", victim);
      act("$n stumbles as something goes wrong with it's projection.", FALSE, ch, 0, 0, TO_ROOM);
      af.modifier = -dice(2, 6);
    } else {
      send_to_char("&+cA massive rush of adrenaline blurs your vision momentarily!\n", victim);
      af.modifier = dice(2, 6);
    }
  }
  if (IS_NPC(ch)) {
    send_to_char("&+cA massive rush of adrenaline blurs your vision momentarily!\n", victim);
    af.modifier = dice(2, 6);
  }

  af.location = APPLY_CON;
  affect_to_char(victim, &af);

  incSkillSubPsp(ch, PSIONIC_ADRENALIZE, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, victim, arg, PSIONIC_ADRENALIZE);
  return;
}

void do_body_control(struct char_data *ch, char *arg, int cmd) {
  struct affected_type af;
  P_char victim;
  int skl_lvl, psp;

  if (!ch) {
    log("assert: bogus param in do_body_control");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_BODY_CONTROL);

  if (psp == 0) {
    send_to_char("You don't know how to adapt your body to hostile environments.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  victim = ParseTarget(ch, arg);

  if (victim != ch) {
    send_to_char("You can only perform that manipulation on your own body.\n", ch);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_BODY_CONTROL);

  if (affected_by_spell(victim, PSIONIC_BODY_CONTROL)) {
    send_to_char("&+rYour body is already protected from the elements.\n", victim);
    return;
  }

  bzero(&af, sizeof (af));
  af.type = PSIONIC_BODY_CONTROL;
  af.duration = MAX(15, GET_SKILL(ch, PSIONIC_BODY_CONTROL));
  SET_CBIT(af.sets_affs, AFF_BODY_CONTROL);
  affect_to_char(ch, &af);

  send_to_char("&+rYou become immune to hostile environments.\n", victim);

  incSkillSubPsp(ch, PSIONIC_BODY_CONTROL, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, 0, arg, PSIONIC_BODY_CONTROL);

  return;
}

void do_catfall(struct char_data *ch, char *arg, int cmd) {
  struct affected_type af;
  P_char victim;
  int skl_lvl, psp;

  if (!ch) {
    log("assert: bogus param in do_catfall");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_CATFALL);

  if (psp == 0) {
    send_to_char("You don't know how to perform that manipulation on your own body.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  victim = ParseTarget(ch, arg);

  if (victim != ch) {
    send_to_char("You can only perform that manipulation on your own body.\n", ch);
    return;
  }

  if (IS_AFFECTED(ch, AFF_CATFALL)) {
    send_to_char("You are already partialy protected from falling.\n", victim);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_CATFALL);

  bzero(&af, sizeof (af));
  af.type = PSIONIC_CATFALL;
  af.duration = MAX(15, GET_SKILL(ch, PSIONIC_CATFALL) >> 1);
  SET_CBIT(af.sets_affs, AFF_CATFALL);
  affect_to_char(ch, &af);

  send_to_char("You feel a weird sensation as your legs reform slightly.\n", victim);
  incSkillSubPsp(ch, PSIONIC_CATFALL, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, victim, arg, PSIONIC_CATFALL);

  return;
}

void do_flesharmor(struct char_data *ch, char *arg, int cmd) {
  P_char victim = 0;
  int skl_lvl, psp;
  struct affected_type af;

  if (!ch) {
    log("assert: bogus param in do_flesharmor");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_FLESH_ARMOR);

  if (psp == 0) {
    send_to_char("You have no idea how to protect yourself that way.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  if (arg) {
    victim = ParseTarget(ch, arg);
    if (victim != ch && victim) {
      send_to_char("You can only project that onto yourself.\n", ch);
      return;
    }
  }

  if (affected_by_spell(ch, PSIONIC_FLESH_ARMOR)) {
    send_to_char("You don't know how to further protect your body in that fashion.\n", ch);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_FLESH_ARMOR);
  if (IS_NPC(ch))
    skl_lvl = (GET_LEVEL(ch) * (3 / 2));

  bzero(&af, sizeof (af));
  af.type = PSIONIC_FLESH_ARMOR;
  af.duration = 10;
  af.modifier = -MAX(10, (GET_LEVEL(ch) / 10) * (skl_lvl / 14));
  af.location = APPLY_AC;
  affect_to_char(ch, &af);
  act("&+cAn invisible force field starts slowly surrounding your body.", TRUE, ch, 0, victim, TO_CHAR);

  if (skl_lvl > 80 && !number(0, (11 - skl_lvl / 10)))
    spell_stone_skin(GET_LEVEL(ch), ch, ch, 0, TRUE);

  incSkillSubPsp(ch, PSIONIC_FLESH_ARMOR, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, victim, arg, PSIONIC_FLESH_ARMOR);

  return;
}

void do_reduction(struct char_data *ch, char *arg, int cmd) {
  P_char victim = 0;
  int skl_lvl, heightmod, psp, weightmod;
  struct affected_type af;

  if (!ch) {
    log("assert: bogus param in do_reduction");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_REDUCTION);

  if (psp == 0) {
    send_to_char("You have absolutely no idea how to change your bodies dimensions.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }


  if (arg) {
    victim = ParseTarget(ch, arg);
    if ((victim != ch) && victim) {
      send_to_char("You can only project that onto yourself.\n", ch);
      return;
    }
  }

  if (affected_by_spell(ch, PSIONIC_REDUCTION) || affected_by_spell(ch, PSIONIC_EXPANSION) || affected_by_spell(ch, SPELL_ENLARGE) || affected_by_spell(ch, SPELL_REDUCE)) {
    send_to_char("That would be rather terminal. You stop your projection.\n", ch);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_REDUCTION);
  if (IS_NPC(ch))
    skl_lvl = (GET_LEVEL(ch) * (3 / 2));

  heightmod = GET_HEIGHT(ch);
  heightmod *= ((float) skl_lvl / 130);

  weightmod = GET_WEIGHT(ch);
  weightmod *= ((float) skl_lvl / 130);

  if (heightmod >= GET_HEIGHT(ch)) {
    log("do_reduction(): height modifier larger then PC's height.");
    dump_core();
  }

  bzero(&af, sizeof (af));
  af.duration = MAX(5, skl_lvl / 6);
  af.type = PSIONIC_REDUCTION;
  af.modifier = -heightmod;
  af.location = APPLY_CHAR_HEIGHT;
  affect_to_char(ch, &af);
  af.modifier = -weightmod;
  af.location = APPLY_CHAR_WEIGHT;
  affect_to_char(ch, &af);

  act("&+cYour body shrinks substantionaly as you complete your projection.&N", TRUE, ch, 0, victim, TO_CHAR);
  act("$n's&+c body shrinks as $s projection completes.&N", TRUE, ch, 0, victim, TO_ROOM);

  incSkillSubPsp(ch, PSIONIC_REDUCTION, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, victim, arg, PSIONIC_REDUCTION);

  return;
}

void do_expansion(struct char_data *ch, char *arg, int cmd) {
  P_char victim = 0;
  int skl_lvl, psp, heightmod, weightmod;
  struct affected_type af;

  if (!ch) {
    log("assert: bogus param in do_expansion");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_EXPANSION);

  if (psp == 0) {
    send_to_char("You have absolutely no idea how to change your bodies dimensions.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  if (arg) {
    victim = ParseTarget(ch, arg);
    if ((victim != ch) && victim) {
      send_to_char("You can only project that onto yourself.\n", ch);
      return;
    }
  }

  if (affected_by_spell(ch, PSIONIC_EXPANSION) || affected_by_spell(ch, PSIONIC_REDUCTION) || affected_by_spell(ch, SPELL_ENLARGE) || affected_by_spell(ch, SPELL_REDUCE)) {
    send_to_char("That would be rather terminal. You stop your projection.\n", ch);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_EXPANSION);
  if (IS_NPC(ch))
    skl_lvl = (GET_LEVEL(ch) * (3 / 2));

  heightmod = GET_HEIGHT(ch);
  heightmod *= ((float) skl_lvl / 130);

  weightmod = GET_WEIGHT(ch);
  weightmod *= ((float) skl_lvl / 130);

  bzero(&af, sizeof (af));
  af.duration = MAX(5, skl_lvl / 6);
  af.type = PSIONIC_EXPANSION;
  af.modifier = heightmod;
  af.location = APPLY_CHAR_HEIGHT;
  affect_to_char(ch, &af);
  af.modifier = weightmod;
  af.location = APPLY_CHAR_WEIGHT;
  affect_to_char(ch, &af);
  affect_total(ch);

  act("&+cYour body grows taller as you complete your projection.&N", TRUE, ch, 0, victim, TO_CHAR);
  act("&+c$n's body grows taller as $s projection completes.&N", TRUE, ch, 0, victim, TO_ROOM);

  incSkillSubPsp(ch, PSIONIC_EXPANSION, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, victim, arg, PSIONIC_EXPANSION);

  return;
}

void do_sustain(struct char_data *ch, char *arg, int cmd) {
  P_char victim;
  int skl_lvl, mvpts, psp;

  if (!ch) {
    log("assert: bogus param in do_sustain");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_SUSTAIN);

  if (psp == 0) {
    send_to_char("You have no idea how to accomplish that.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  if (arg) {
    victim = ParseTarget(ch, arg);
    if (victim != ch && victim) {
      send_to_char("You can only project that onto yourself.\n", ch);
      return;
    }
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_SUSTAIN);

  mvpts = 40 + dice(1, 10);

  if (GET_MOVE(ch) > mvpts) {
    /* If they are full ie more then 25 units, wont do squat but lets give
     * em the message for simplicity's sake */
    GET_MOVE(ch) -= mvpts;
    send_to_char("&+cYou transfer some of your energies to replenish your metabolic system.\n", ch);
    send_to_char("&+cYou feel considerably more tired.\n", ch);
    if (GET_COND(ch, FULL) < 24)
      GET_COND(ch, FULL) += (15 + skl_lvl / 10);
    if (GET_COND(ch, THIRST) < 24)
      GET_COND(ch, THIRST) += (15 + skl_lvl / 10);
  } else {
    send_to_char("&+cUnfortunatelly you feel too tired to complete this projection.\n", ch);
    send_to_char("&+cYou abort abruptly.\n", ch);
  }

  incSkillSubPsp(ch, PSIONIC_SUSTAIN, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, victim, arg, PSIONIC_SUSTAIN);

  return;
}

void do_equalibrium(struct char_data *ch, char *arg, int cmd) {
  P_char victim;
  int skl_lvl, psp;
  int poison = FALSE, curse = FALSE, paralyze = FALSE;
  int maj_paralyze = FALSE, blind = FALSE, disease = FALSE;
  int used = FALSE;

  if (!ch) {
    log("assert: bogus param in do_equalibrium");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_EQUALIBRIUM);

  if (psp == 0) {
    send_to_char("You have no idea how to accomplish that.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  if (arg)
    if ((ch != ParseTarget(ch, arg))) {
      send_to_char("You can only project that upon yourself.\n", ch);
      return;
    }

  skl_lvl = GET_SKILL(ch, PSIONIC_EQUALIBRIUM);

  if (GET_LEVEL(ch) > 10)
    poison = TRUE;
  if (GET_LEVEL(ch) > 15)
    curse = TRUE;
  if (GET_LEVEL(ch) > 20) {
    disease = TRUE;
    paralyze = TRUE;
  }
  if (GET_LEVEL(ch) > 25) {
    if (IS_PC(ch) && (skl_lvl >= 60))
      blind = TRUE;
    else if (IS_NPC(ch))
      blind = TRUE;
  }
  if (GET_LEVEL(ch) > 40) {
    if (IS_PC(ch) && (skl_lvl >= 80))
      maj_paralyze = TRUE;
    else if (IS_NPC(ch))
      maj_paralyze = TRUE;
  }

  if ((poison) && (affected_by_spell(ch, SPELL_POISON) || affected_by_spell(ch, SPELL_SLOW_POISON))) {
    act("Your projection has neutralized the poison in your bloodstream.", FALSE, ch, 0, victim, TO_CHAR);
    if (affected_by_spell(ch, SPELL_SLOW_POISON))
      affect_from_char(ch, SPELL_SLOW_POISON);
    if (affected_by_spell(ch, SPELL_POISON))
      affect_from_char(ch, SPELL_POISON);
    used = TRUE;
  }
  if ((curse) && (affected_by_spell(ch, SPELL_CURSE))) {
    act("You analyze the nature of the curse and attempt to remove it.", FALSE, ch, 0, victim, TO_CHAR);
    spell_remove_curse(50, ch, ch, 0);
    used = TRUE;
  }
  if ((paralyze) && (IS_AFFECTED(ch, AFF_MINOR_PARALYSIS))) {
    act("Blood begins to flow freely, as your joints loosen up.", FALSE, ch, 0, victim, TO_CHAR);
    REMOVE_CBIT(ch->specials.affects, AFF_MINOR_PARALYSIS);
    affect_from_char(ch, SPELL_MINOR_PARALYSIS);
    used = TRUE;
  }
  if ((maj_paralyze) && (IS_AFFECTED(ch, AFF_MAJOR_PARALYSIS))) {
    act("Blood begins to flow freely, as your joints loosen up.", FALSE, ch, 0, victim, TO_CHAR);
    REMOVE_CBIT(ch->specials.affects, AFF_MAJOR_PARALYSIS);
    affect_from_char(ch, SPELL_MAJOR_PARALYSIS);
    used = TRUE;
  }
  if ((blind) && (IS_AFFECTED(ch, AFF_BLIND))) {
    act("Your vision returns, just as you willed it to.", FALSE, ch, 0, victim, TO_CHAR);
    spell_cure_blind(50, ch, ch, 0);
    used = TRUE;
  }

  if (!used)
    send_to_char("There seems to be nothing wrong with your body at the moment.\n", ch);

  incSkillSubPsp(ch, PSIONIC_EQUALIBRIUM, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, victim, arg, PSIONIC_EQUALIBRIUM);

  return;
}

/*****************************************************************************/
/*                                                                           */
/*                         P S Y C H O P O R T A T I O N                     */
/*                                                                           */

/*****************************************************************************/
void do_shift(struct char_data *ch, char *arg, int cmd) {
  P_char t_ch, ch1 = NULL, ch2 = NULL;
  P_char srcChar = NULL, destChar = NULL;
  int psp;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  arg1[0] = 0;
  arg2[0] = 0;

  if (!ch) {
    log("assert: bogus parm in do_shift");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_SHIFT);

  if (psp == 0) {
    send_to_char("You have no idea how to do that. Looks like you'll have to walk.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  half_chop(arg, arg1, arg2);

  if (!*arg1) {
    send_to_char("Shift who what where?\n", ch);
    return;
  }

  if (*arg)
    argument_interpreter(arg, arg1, arg2);
  else {
    send_to_char("Shift who what where?\n", ch);
    return;
  }

  argument_interpreter(arg, arg1, arg2);

  /* Check for the first char (either src of dest char) validity */
  if (*arg1) {
    ch1 = get_char_in_game_vis(ch, arg1, FALSE);
    if (!ch1 || !IS_PC(ch1)) {
      send_to_char("You fail to locate your targets.\n", ch);
      return;
    }

    // Removed -- CRM
    if (ch1 != ch) { /* This allows: teleport CH PC command format as CH doesnt need his own consent.. */
      if (((ch1->specials.consent != ch) && !IN_ACHERON(ch1)) && !IS_TRUSTED(ch) && IS_PC(ch)) {
        act("You do not have $N's consent.", TRUE, ch, 0, ch1, TO_CHAR);
        act("You have a sensation of something tugging at your body. It stops suddenly."
                , TRUE, ch, 0, ch1, TO_VICT);
        return;
      }
    }

    if (ch1 != ch) {
      if (RACE_GOOD(ch1) && !IN_ACHERON(ch1) && IS_PC(ch)) {
        send_to_char("You fail.\n", ch);
        return;
      }
    }
  }

  /* Check for the second char validity */
  if (*arg2) {
    ch2 = get_char_in_game_vis(ch, arg2, FALSE);
    if (!ch2 || !IS_PC(ch2)) {
      send_to_char("You fail to locate your targets.\n", ch);
      return;
    }
    if (((ch1->specials.consent != ch) && !IN_ACHERON(ch2)) && !IS_TRUSTED(ch) && IS_PC(ch)) {
      act("You do not have $N's consent.", TRUE, ch, 0, ch1, TO_CHAR);
      return;
    }
  }

  /*   At this point we got:
   *     - Sources and Target's consents
   *     - Neither is a NPC
   *     - Src and Target are valid PC's
   */
  if (!ch2) {
    /* First Case:  The Caster to another PC   */
    if (ch1 == ch || ch->in_room == ch1->in_room) {
      send_to_char("But you are already here!.\n", ch);
      return;
    }

    if ((IN_ACHERON(ch) && !IN_ACHERON(ch1)) ||
            (IN_ACHERON(ch1) && !IN_ACHERON(ch))) {
      send_to_char("You fail.\n", ch);
      return;
    }
    /* saves some figuring out who to send the messages later on */
    srcChar = ch;
    destChar = ch1;
  } else {
    /* Second Case: PC1 to PC2 */

    if (ch->in_room != ch1->in_room) {
      send_to_char("You have to see the person you are trying to shift in order for this to work.\n", ch);
      return;
    }
    if (ch1->in_room == ch2->in_room) { /*Dont give clues whether two PC's are together in the same room*/
      send_to_char("Something prevents your projections from completing.\n", ch);
      return;
    }
    if ((IN_ACHERON(ch) && !IN_ACHERON(ch2)) ||
            (IN_ACHERON(ch2) && !IN_ACHERON(ch))) {
      send_to_char("You fail.\n", ch);
      return;
    }

    srcChar = ch1;
    destChar = ch2;
  }

  if (plane_of_room(destChar->in_room) != plane_of_room(srcChar->in_room)) {
    send_to_char("You fail to find your target on this plane.\n", ch);
    return;
  }

  if (IS_CSET(world[srcChar->in_room].room_flags, NO_TELEPORT) ||
          IS_CSET(world[srcChar->in_room].room_flags, PRIVATE) ||
          IS_CSET(world[srcChar->in_room].room_flags, MAX_TWO_PC) ||
          IS_CSET(world[srcChar->in_room].room_flags, PRIV_ZONE) ||
          IS_SET(zone_table[world[srcChar->in_room].zone].flags, ZONE_NO_TELE) ||
          IS_CSET(world[destChar->in_room].room_flags, NO_TELEPORT) ||
          IS_CSET(world[destChar->in_room].room_flags, PRIVATE) ||
          IS_CSET(world[destChar->in_room].room_flags, MAX_TWO_PC) ||
          IS_CSET(world[destChar->in_room].room_flags, PRIV_ZONE) ||
          IS_SET(zone_table[world[destChar->in_room].zone].flags, ZONE_NO_TELE)) {
    send_to_char("Something prevents your projections from completing.\n", ch);
    return;
  }

  if (GET_LEVEL(srcChar) < 20) {
    send_to_char("You cannot shift players under 20th level.", ch);
    return;
  }

  /******************TELEPORT*******************/
  act("$n &+Lis engulfed by a black shimmering shadow and dissipates into\n"
          "&+Lnothingness.", FALSE, srcChar, 0, 0, TO_ROOM);
  act("&+LA black shimmering shadow appears nearby. It dissipates rapidly\n"
          "&+Lrevealing $N.&N", FALSE, destChar, 0, srcChar, TO_ROOM);
  act("&+LA black shimmering shadow appears nearby. It dissipates rapidly\n"
          "&+Lrevealing $N &+Lgrinning madly at you.&N", FALSE, destChar, 0, srcChar, TO_CHAR);


  if (!IS_AFFECTED(srcChar, AFF_BLIND)) {
    act("&+LEverything shimmers madly as you are enveloped by blackness.\n"
            "&+LSuddenly the darkness falls revealing different surroundings.\n"
            , FALSE, srcChar, 0, 0, TO_CHAR);
  } else {
    act("&+LYou feel distinctly strange for a couple moments.\n&+LThe sense of being displaced passes quickly.&N"
            , FALSE, srcChar, 0, 0, TO_CHAR);
  }

  if (IS_FIGHTING(srcChar))
    stop_fighting(srcChar);
  if (IS_RIDING(srcChar))
    stop_riding(srcChar);

  if (srcChar->in_room != NOWHERE)
    for (t_ch = world[srcChar->in_room].people; t_ch; t_ch = t_ch->next)
      if (IS_FIGHTING(t_ch) && (t_ch->specials.fighting == srcChar))
        stop_fighting(t_ch);
  char_from_room(srcChar);
  char_to_room(srcChar, destChar->in_room, -1);

  if (IS_CSET(world[destChar->in_room].room_flags, DEATH) && (GET_LEVEL(srcChar) < MINLVLIMMORTAL)) {
    die_deathtrap(srcChar);
  }

  if (srcChar == ch)
    incSkillSubPsp(ch, PSIONIC_SHIFT, psp, PSIONIC_GAIN_ON);
  else
    incSkillSubPsp(ch, PSIONIC_SHIFT, psp << 1, PSIONIC_GAIN_ON);

  psiSkillUsageLogging(ch, 0, arg, PSIONIC_SHIFT);

  return;

}

/* Basicaly a cast_gate function but modified to fit a skill call format    */
void do_rift(struct char_data *ch, char *arg, int cmd) {
  struct obj_data * rift1 = NULL, rift2 = NULL, gate_block = NULL;
  char Gbuf4[MAX_STRING_LENGTH];
  int tries, to_room, plane_to, plane_from, g1, g2, skl_lvl, psp;

  if (!ch || (ch->in_room == NOWHERE)) {
    log("assert: bogus parmeters in do_rift");
    return;
  }

  psp = canUsePsionicSkill(ch, PSIONIC_RIFT);

  if (psp == 0) {
    send_to_char("You have no idea how to do that. Looks like you'll have to walk.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  if (IS_CSET(world[ch->in_room].room_flags, NO_TELEPORT) ||
          IS_SET(zone_table[world[ch->in_room].zone].flags, ZONE_NO_TELE) ||
          IN_ACHERON(ch)) {
    send_to_char("The rift opens for a brief second and then closes.\n", ch);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_RIFT);

  one_argument(arg, Gbuf4);

  for (plane_to = 0; plane_to < nr_planes; plane_to++)
    if (is_abbrev(Gbuf4, plane_info[plane_to].keyword))
      break;

  if ((plane_to >= nr_planes) || (!IS_TRUSTED(ch) && ((plane_to == 1) || (plane_to == 2) || (plane_to == 7) || (plane_to == 8) || (plane_to == 9) || (plane_to == 11) || (plane_to == 12) || (plane_to == 14)))) {
    send_to_char("You can open a planar rift only to: Astral, Ethereal, Air, Fire, Smoke or Prime planes!\n", ch);
    return;
  }

  /* have our target plane in plane_to, now we have to find out what plane we are currently inhabiting */

  plane_from = plane_of_room(ch->in_room);

  if (plane_from == NOWHERE)
    return;

  if (plane_to == plane_from) {
    send_to_char("It's impossible to open a planar rift to the same plane!\n", ch);
    send_to_char("You can open a planar rift only to: Astral, Ethereal, Air, Fire or Prime planes!\n", ch);
    return;
  }

  if (plane_to) {
    g1 = real_room(plane_info[plane_to].gate_start);
    g2 = real_room(plane_info[plane_to].gate_end);
  } else {
    g1 = zone_table[3].real_bottom + 1;
    g2 = top_of_world;
  }
  if ((g1 == NOWHERE) || (g2 == NOWHERE) || (g1 > g2)) {
    log(LOG_DEBUG, "Bleah, plane_info is hosed (do_rift)");
    send_to_char("Looks like the planes data is out of date, dimension rift unusable until it's fixed.\n", ch);
    return;
  }

  /* Look for the gate blocking object, o 898.  Kick out if true */
  for (gate_block = world[g1].contents;
          gate_block; gate_block = gate_block->next_content) {
    if (obj_index[gate_block->R_num].virtual == ATD_GATE_BLOCK) {
      send_to_char("A planar rift opens for a brief second and then closes.\n", ch);
      return;
      break;
    }
  }

  for (tries = skl_lvl / 10; tries; tries--) {
    to_room = number(g1, g2);
    if (IS_CSET(world[to_room].room_flags, NO_MAGIC) ||
            IS_CSET(world[to_room].room_flags, RESERVED_OLC) ||
            IS_CSET(world[to_room].room_flags, NO_TELEPORT) ||
            IS_SET(zone_table[world[to_room].zone].flags, ZONE_NO_MAGIC) ||
            IS_SET(zone_table[world[to_room].zone].flags, ZONE_NO_TELE))
      continue;
    if ((world[to_room].sector_type == SECT_OCEAN) ||
            (world[to_room].sector_type == SECT_NO_GROUND) ||
            (world[to_room].sector_type == SECT_WATER_SWIM) ||
            (world[to_room].sector_type == SECT_WATER_NOSWIM) ||
            (world[to_room].sector_type == SECT_UNDRWLD_NOSWIM) ||
            (world[to_room].sector_type == SECT_UNDRWLD_WATER) ||
            (world[to_room].sector_type == SECT_UNDRWLD_NOGROUND) ||
            (world[to_room].sector_type == SECT_UNDERWATER) ||
            (world[to_room].sector_type == SECT_UNDERWATER_GR) ||
            (world[to_room].sector_type == SECT_UD_UNDERWATER) ||
            (world[to_room].sector_type == SECT_UD_UNDERWATER_GR))
      continue;
    if (plane_of_room(to_room) != plane_to)
      continue;
    break;
  }

  if (tries <= 0) {
    send_to_char("A planar rift opens for a brief second and then closes.\n", ch);
    return;
  }

  if (!(rift1 = read_object(753, VIRTUAL))) {
    send_to_char("Planar rift objects are messed up. Report to an admin.\n", ch);
    return;
  }
  if (!(rift2 = read_object(753, VIRTUAL))) {
    send_to_char("Planar rift objects messed up. Report to an admin.\n", ch);
    free_obj(rift1);
    return;
  }

  if (world[to_room].people) {
    act("&+LA jet black, &N&+rshimmering&N&+L vortex opens with a slow rumble.",
            0, world[to_room].people, rift1, 0, TO_ROOM);
    act("&+LA jet black, &N&+rshimmering&N&+L vortex opens with a slow rumble.",
            0, world[to_room].people, rift1, 0, TO_CHAR);
  }
  act("&+LA jet black, &N&+rshimmering&N&+L vortex opens with a slow rumble.",
          0, ch, rift2, 0, TO_ROOM);
  act("&+LA jet black, &N&+rshimmering&N&+L vortex opens with a slow rumble.",
          0, ch, rift2, 0, TO_CHAR);

  /* value[0] = destination room */
  rift1->value[0] = world[ch->in_room].number;
  rift2->value[0] = world[to_room].number;

  obj_to_room(rift1, to_room);
  obj_to_room(rift2, ch->in_room);
  AddEvent(EVENT_DECAY, (10 + (skl_lvl / 25)) * PULSE_VIOLENCE, TRUE, rift1, 0);
  AddEvent(EVENT_DECAY, (10 + (skl_lvl / 25)) * PULSE_VIOLENCE, TRUE, rift2, 0);
  sprintf(Gbuf4, "You open a planar rift to  %s plane!\n", plane_info[plane_to].keyword);
  send_to_char(Gbuf4, ch);

  incSkillSubPsp(ch, PSIONIC_RIFT, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, 0, arg, PSIONIC_RIFT);

  return;

}

/****************************************************************************/
/*                                                                          */
/*                           T E L E P A T H Y                              */
/*                                                                          */
/****************************************************************************/

/* modifier used to be 4, I upped it to 6 because the number of squid pets
 * was getting ridiculous.  I also yanked their ability to dom MU or CL mobs
 * because it was waaaay out of line.  And if I hear another one of the little
 * bastards complain "waaaah, I can't solo shit anymore!", heads are gonna
 * roll.  -- D2
 *
 * hehe kill em all! -- Alth
 */

#define DOMINATE_MODIFIER  6

void do_dominate(struct char_data *ch, char *arg, int cmd) {
  P_char victim;
  int psp, maxPets;

  if (!(ch)) {
    log("assert: bogus parms in do_dominate");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_DOMINATE);

  if (psp == 0) {
    send_to_char("Dominate? You wouldn't be able to dominate someone if they paid you.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  if (arg)
    victim = ParseTarget(ch, arg);
  else {
    send_to_char("Dominate who?\n", ch);
    return;
  }

  /* Run all the approperiate actor only checks before engaging  */
  if (!AWAKE(ch)) {
    send_to_char("How can you dominate someone while sleeping?\n", ch);
    return;
  }
  if (affected_by_spell(ch, SONG_PEACE)) {
    send_to_char("You feel way too peaceful to consider doing anything offensive!\n", ch);
    return;
  }
  if (IS_AFFECTED(ch, AFF_STUNNED)) {
    send_to_char("You're too stunned to even contemplate projecting anything!\n", ch);
    return;
  }
  if (IS_AFFECTED(ch, AFF_KNOCKED_OUT)) {
    send_to_char("You can't do much of anything while knocked out!\n", ch);
    return;
  }
  if (CHAR_IN_SAFE_ZONE(ch)) {
    send_to_char("You feel ashamed trying to disrupt the tranquility of this place.\n", ch);
    return;
  }

  if (victim == ch) {
    send_to_char("Aren't we funny today..\n", ch);
    return;
  }

  maxPets = (GET_SKILL(ch, PSIONIC_DOMINATE) / DOMINATE_MODIFIER);

  if (getNumberOfPets(ch) >= maxPets) {
    send_to_char("You aren't strong enough to control more followers.\n", ch);
    return;
  }

  if (victim) {
    /* Check for single file room.  If PC isn't next to victim, don't let them
       do it. DMB 10/11/98 */

    if (IS_CSET(world[ch->in_room].room_flags, SINGLE_FILE) && !AdjacentInRoom(ch, victim)) {
      send_to_char("You can't get a clear line of sight in these cramped quarters.\n", ch);
      return;
    } else {
      dominate_single(ch, victim, GET_SKILL(ch, PSIONIC_DOMINATE));
      incSkillSubPsp(ch, PSIONIC_DOMINATE, psp, PSIONIC_GAIN_ON);
      psiSkillUsageLogging(ch, victim, arg, PSIONIC_DOMINATE);
    }
  } else
    send_to_char("Dominate who?\n", ch);


  return;
}

void do_mass_domination(struct char_data *ch, char *arg, int cmd) {
  P_char tmp_victim, temp;
  int skl_lvl, psp;
  int maxFollowers = 0;
  int numFollowers = 0;

  if (!(ch)) {
    log("assert: bogus parms");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_MASS_DOMINATION);

  if (psp == 0) {
    send_to_char("Dominate? You wouldn't be able to dominate someone if they paid you.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  if (!AWAKE(ch)) {
    send_to_char("How can you dominate someone while sleeping?\n", ch);
    return;
  }
  if (affected_by_spell(ch, SONG_PEACE)) {
    send_to_char("You feel way too peaceful to consider doing anything offensive!\n", ch);
    return;
  }
  if (IS_AFFECTED(ch, AFF_STUNNED)) {
    send_to_char("You're too stunned to even contemplate projecting anything!\n", ch);
    return;
  }
  if (IS_AFFECTED(ch, AFF_KNOCKED_OUT)) {
    send_to_char("You can't do much of anything while knocked out!\n", ch);
    return;
  }
  if (CHAR_IN_SAFE_ZONE(ch)) {
    send_to_char("You feel ashamed trying to disrupt the tranquility of this place.\n", ch);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_MASS_DOMINATION);
  maxFollowers = skl_lvl / DOMINATE_MODIFIER;
  numFollowers = getNumberOfPets(ch);

  if (numFollowers >= maxFollowers) {
    send_to_char("You aren't strong enough to control more followers.\n", ch);
    return;
  }

  for (tmp_victim = world[ch->in_room].people; tmp_victim; tmp_victim = temp) {
    temp = tmp_victim->next_in_room;
    if (IS_NPC(tmp_victim)) {
      if (dominate_single(ch, tmp_victim, skl_lvl))
        numFollowers++;
    }
  }

  incSkillSubPsp(ch, PSIONIC_MASS_DOMINATION, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, 0, arg, PSIONIC_MASS_DOMINATION);
}

/* Again a slight modification of spell_charm_person to suit the squids.. */
/* Domination can charm non humanoids as its based more on the control    */
/* over someones brain, instead of the human charm, eye to eye, mumbo jumbo */

/* Does not do any checks to see whether ch can pull that stunt on a victim
 * That stuff is supposed to be done in the calling routing.. Slight
 * performance improvement that way..                                       */
int dominate_single(struct char_data *ch, P_char victim, int skill_level) {
  bool failed = FALSE;
  int i, numFollowers, maxFollowers;
  struct affected_type af;
  P_event e1;

  if (!ch || !victim) {
    log("assert: bogus params in do_dominate");
    dump_core();
  }

  if (victim == ch)
    return FALSE;

  if (victim->following == ch)
    return FALSE;

  if (GET_STAT(victim) == STAT_DEAD)
    return FALSE;

  if (nokill(ch, victim))
    return FALSE;

  if (should_not_kill(ch, victim))
    return FALSE;

  if (victim == ch) {
    send_to_char("Ooooh you are sooo charming!\n", ch);
    return FALSE;
  }
  if (circle_follow(victim, ch)) {
    send_to_char("Sorry, following in circles cannot be allowed.\n", ch);
    return FALSE;
  }
  if (CHAR_IN_SAFE_ZONE(ch)) {
    send_to_char("Something blocks your projection.\n", ch);
    return FALSE;
  }

  maxFollowers = skill_level / DOMINATE_MODIFIER;
  numFollowers = getNumberOfPets(ch);

  if (numFollowers >= maxFollowers) {
    send_to_char("You aren't strong enough to control more followers.\n", ch);
    return FALSE;
  }

  /* see if the NPC belongs(ed) to someone else, dont allow dominating on principle */
  if (IS_NPC(victim)) {
    if (victim->psn != 0)
      return FALSE;

    /* check if we already have a save_event on the NPC victim, if so dont allow */
    LOOP_EVENTS(e1, victim->events) {
      if (e1->type == EVENT_AUTO_SAVE)
        return FALSE;
    }
  }

  if (!CAN_SEE(ch, victim))
    failed = TRUE;

  for (i = 0; (i < MAX_WEAR) && !failed; i++)
    if (victim->equipment[i] && IS_SET(victim->equipment[i]->extra_flags, ITEM_NOCHARM))
      failed = TRUE;

  /* this CAPS the level of the victim below 25, thus no free full heals or stones */
  if (GET_LEVEL(ch) < (GET_LEVEL(victim) << 1))
    failed = TRUE;

  /* bah, the hell with that.  PCs shouldn't be using dom'ed mobs for ANY
     spells.  At most, a tank or lure, but mostly as food.  Thus, don't let
     them dom mage/cleric/psi mobs.  */
  if (IS_MAGE(victim) || IS_CLERIC(victim) || IS_PSIONICIST(victim))
    failed = TRUE;

  if (!failed && IS_AFFECTED(victim, AFF_CHARM)) {
    /* several possibilities if victim is already charmed */
    if (!victim->following) {
      /* should only happen when bit is set by other than spell */
      failed = TRUE;
    } else if (GET_LEVEL(ch) > (int) GET_LEVEL(victim->following)) {
      /* victim gets another save versus initial charm (with bonus) */
      if (NewSaves(victim, SAVING_PARA, GET_LEVEL(victim->following) - GET_LEVEL(ch))) {
        /* quietly */
        affect_from_char(victim, SPELL_CHARM_PERSON);
      } else
        failed = TRUE;
    } else
      failed = TRUE;
  }
  if (!failed && saves_spell(victim, SAVING_PARA))
    failed = TRUE;

  if (failed) {
    if (CAN_SEE(ch, victim))
      appear(ch);
    send_to_char("Your victim's mind resists your attempts at domination.\n", ch);
    act("$n tried to dominate you, but failed!", FALSE, ch, 0, victim, TO_VICT);

    if (CAN_SEE(victim, ch)) {
      /* if they fail, wham! */
      if (IS_NPC(victim) && HAS_MEMORY(victim) && IS_PC(ch) &&
              !IS_CSET(ch->only.pc->pcact, PLR_AGGIMMUNE))
        mem_addToMemory(victim->only.npc->memory, GET_NAME(ch));
      hit(victim, ch, TYPE_UNDEFINED);
    }
    return FALSE;
  }
  /* if get here, spell worked, so do the nasty thing */

  if (victim->following && (victim->following != ch))
    stop_follower(victim);

  if (!victim->following)
    add_follower(victim, ch, TRUE);

  bzero(&af, sizeof (af));

  af.type = SPELL_CHARM_PERSON;
  af.duration = skill_level;
  SET_CBIT(af.sets_affs, AFF_CHARM);
  affect_to_char(victim, &af);

  act("&+LYour will disappears abruptly as $n takes control of your mind...", FALSE, ch, 0, victim, TO_VICT);
  appear(ch);

  if ((IS_FIGHTING(ch)) && (ch->specials.fighting == victim))
    stop_fighting(ch);

  if (IS_FIGHTING(victim))
    stop_fighting(victim);

  StopMercifulAttackers(victim);

  return TRUE;
}

void do_synaptic_static(struct char_data *ch, char *arg, int cmd) {
  int skl_lvl, i, tmp_lvl, success = 0, circle, psp;
  P_char victim;

  if (!ch) {
    log("assert: bogus parm in do_mind_wipe");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_SYNAPTIC_STATIC);

  if (psp == 0) {
    send_to_char("You have no idea how to make someone forget anything.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  victim = ParseTarget(ch, arg);

  if (!victim) {
    send_to_char(" Whose memory do you want to scramble?\n", ch);
    return;
  }

  /* Check for single file room.  If PC isn't next to victim, don't let them
     do it. DMB 10/11/98 */
  if (IS_CSET(world[ch->in_room].room_flags, SINGLE_FILE) && !AdjacentInRoom(ch, victim)) {
    send_to_char("You can't get a clear line of sight in these cramped quarters.\n", ch);
    return;
  }

  if (ch == victim) {
    send_to_char("That would have disasterous effects.\nYou stop your projection.\n", ch);
    return;
  }

  /* unimplemented yet, making PC's forget spells is handled in a diff way */
  if (IS_PC(victim))
    return;

  if (!CanPsiDoFightMove(ch, victim))
    return;

  skl_lvl = GET_SKILL(ch, PSIONIC_SYNAPTIC_STATIC);
  tmp_lvl = skl_lvl / 10;
  for (i = 1; i <= tmp_lvl; i++) {
    circle = number(1, tmp_lvl);
    if (victim->only.npc->spells_in_circle[circle] > 0) {
      victim->only.npc->spells_in_circle[circle]--;
      victim->only.npc->spells_in_circle[0]++;
      success++;
    }
  }

  if (success) {
    act("You suddenly feel a flash of blinding pain as $n does something to your mind.", TRUE, ch, 0, victim, TO_VICT);
    act("Your projection successfully scrambles $N thoughts and memories.", TRUE, ch, 0, victim, TO_CHAR);
    act("$N's eyes cloud over for a moment.", TRUE, ch, 0, victim, TO_ROOM);
  } else {
    act("Your attempts to scramble $N's mind fail miserably.", TRUE, ch, 0, victim, TO_CHAR);
    act("$n's projection fails miserably as $s tries to scramble your thoughts.", TRUE, ch, 0, victim, TO_VICT);
  }

  if (!IS_FIGHTING(victim))
    hit(victim, ch, TYPE_UNDEFINED);

  incSkillSubPsp(ch, PSIONIC_SYNAPTIC_STATIC, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, victim, arg, PSIONIC_SYNAPTIC_STATIC);

  return;
}

void do_tower_of_iron_will(struct char_data *ch, char *arg, int cmd) {
  struct affected_type af;
  P_char victim;
  int psp;

  if (!ch) {
    log("assert: bogus param in do_tower_of_iron_will");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_TOWER_OF_IRON_WILL);

  if (psp == 0) {
    send_to_char("You don't know how to protect yourself from psionic "
            "attack.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  if (arg) {
    victim = ParseTarget(ch, arg);
    if ((victim != ch) && victim) {
      send_to_char("You can only perform that manipulation on your own body.\n", ch);
      return;
    }
  }

  if (IS_AFFECTED(ch, AFF_TOWER_OF_IRON_WILL)) {
    send_to_char("You are already protected from psionic attacks!\n", ch);
    return;
  }

  if (!affected_by_spell(ch, PSIONIC_TOWER_OF_IRON_WILL))
    send_to_char("&+BYou envelop your mind in a protective tower.&N\n", ch);

  bzero(&af, sizeof (af));
  af.type = PSIONIC_TOWER_OF_IRON_WILL;
  af.duration = 3;
  SET_CBIT(af.sets_affs, AFF_TOWER_OF_IRON_WILL);
  affect_to_char(ch, &af);

  incSkillSubPsp(ch, PSIONIC_TOWER_OF_IRON_WILL, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, victim, arg, PSIONIC_TOWER_OF_IRON_WILL);

  return;
}

void do_attraction(struct char_data *ch, char *arg, int cmd) {
}

void do_alter_aura(struct char_data *ch, char *arg, int cmd) {
}

void do_enhance_skill(struct char_data *ch, char *arg, int cmd) {
}

void do_canibalize(struct char_data *ch, char *arg, int cmd) {
  int can_use, skl_lvl, moves, ratio, psps, time_to_rest;
  char oneArg[MAX_STRING_LENGTH];

  if (!ch) {
    log("assert: bogus param in do_canibalize");
    dump_core();
  }

  can_use = canUsePsionicSkill(ch, PSIONIC_CANIBALIZE);

  if (can_use == 0) {
    send_to_char("Canibalize? But you aren't even hungry.\n", ch);
    return;
  }

  if (can_use == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  if (!CHECK_PSIONIC_USAGE(ch, TIMED_USAGE_CANIBALIZE)) {
    time_to_rest = PSIONIC_HOURS_TO_REST(ch, TIMED_USAGE_CANIBALIZE);
    if (time_to_rest > 5)
      send_to_char("Your body needs quite a bit more time to regenerate.\n", ch);
    else if (time_to_rest <= 1)
      send_to_char("You feel almost ready again.\n", ch);
    else
      send_to_char("You need a little bit more time to regenerate.\n", ch);
    return;
  } else {
    if (!*arg) {
      send_to_char("You are ready to transfer your energies.\n", ch);
      return;
    }
  }

  /* This is the fixed version of getting the proficiency - Iyachtu */
  skl_lvl = GET_SKILL(ch, PSIONIC_CANIBALIZE);

  one_argument(arg, oneArg);

  if (!*oneArg || !isdigit(*oneArg)) {
    send_to_char("Hrmmm, how many move points did you want to canibalize again?\n", ch);
    return;
  }

  moves = atoi(oneArg);

  if (moves <= 0) {
    send_to_char("Hrmmm, how many move points did you want to canibalize again?\n", ch);
    return;
  }

  if (GET_MOVE(ch) <= 0) {
    send_to_char("You cannot find any available energy to canibalize.\nYou abort your projection.\n", ch);
    return;
  }

  /* Ratio of MVs converted to PSPs
   *
   * 100% skill will have a  1:1 ratio
   * 50%  skill will have a  6:1 ratio
   * 10%  skill will have a 10:1 ratio etc  */

  /* This formula makes the skill worthless, as the simplest squid skill costs
     at least 15 psp's.  New formula below will range the conversion percentage
     from 30% to 160%, and if their skill is greater than 95 it will be 200%.
     Put THAT in your pipe and smoke it.  --D2 */

  if ((skl_lvl > 0) && (skl_lvl < 30))
    skl_lvl = 30;

  ratio = (skl_lvl * 2) - 30;

  /* Added the +1 in the following line so can actually get to 1-1 at 99 skill level - Iyachtu */
  ratio = 11 - ((skl_lvl + 1) / 10);

  if (skl_lvl > 95)
    ratio = 200;

  if (GET_MOVE(ch) < moves) {
    send_to_char("You canibalize your body of all the available energy you can find.\n", ch);
    moves = GET_MOVE(ch);
  }

  /* The first 100 movement points are converted using the ratio
   * the remaining moves are converted using the maximum ratio,
   * a cap to prevent people with huge moves from getting too much
   * out of it would be kinda silly, so everything over 100 moves gets
   * converted using 10:1 ratio..                                      */

  if (moves <= 100)
    psps = moves / ratio;
  else
    psps = (100 / ratio) + ((moves - 100) / 10) + number(1, 5);

  psps = moves * ratio / 100;

  /* If a small amount of moves didnt successfully convert into any psps,
   * rob em of their moves anyways..                                     */
  if (psps > 0)
    send_to_char("You become more focused as your projection completes.\n", ch);
  else
    send_to_char("There wasn't enough available energy in your body to successfully\ncomplete your projection.\n", ch);

  if (GET_PSP(ch) >= GET_MAX_PSP(ch))
    send_to_char("Your projection has no effect.\n", ch);
  else {
    GET_PSP(ch) += psps;
    if (GET_PSP(ch) >= GET_MAX_PSP(ch))
      GET_PSP(ch) = GET_MAX_PSP(ch);
  }

  GET_MOVE(ch) -= moves;
  UPDATE_PSIONIC_USAGE(ch, TIMED_USAGE_CANIBALIZE);

  incSkillSubPsp(ch, PSIONIC_CANIBALIZE, 0, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, 0, arg, PSIONIC_CANIBALIZE);

  return;
}

void do_stasis_field(struct char_data *ch, char *arg, int cmd) {
  int skl_lvl, psp;
  struct affected_type af;

  if (!ch) {
    log("assert: bogus param in do_stasis_field");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_STASIS_FIELD);

  if (psp == 0) {
    send_to_char("Yes, it can be done. Unfortunately you have no idea how.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_STASIS_FIELD);

  if (ch->in_room == NOWHERE)
    return;

  /* See if there is another proc on the room.. Takes care of wiping out
   * some other proc or adding another stasis field.. */

  if (IS_AFFECTED(ch, AFF_STASIS_FIELD)) {
    send_to_char("You are already maintaining a stasis field.\n", ch);
    return;
  }

  bzero(&af, sizeof (af));
  CLEAR_CBITS(af.sets_affs, AFF_BYTES);
  af.type = PSIONIC_STASIS_FIELD;
  af.duration = skl_lvl / 4;
  SET_CBIT(af.sets_affs, AFF_STASIS_FIELD);
  affect_to_char(ch, &af);

  act("&+WA translucent sphere forms all of a sudden centered about $n.\n", FALSE, ch, 0, 0, TO_ROOM);
  send_to_char("&+WAn large translucent sphere forms around you.\n", ch);

  AddEvent(EVENT_CHAR_EXECUTE, 2 * PULSE_VIOLENCE, TRUE, ch, createStasisSphear);

  incSkillSubPsp(ch, PSIONIC_STASIS_FIELD, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, 0, arg, PSIONIC_STASIS_FIELD);

  return;
}

void createStasisSphear(struct char_data *ch) {
  P_char t_char;

  if (!ch) {
    log("assert: bogus parameter in Event Call to createStasisSphear");
    dump_core();
  }

  if (ch->in_room == NOWHERE) {
    send_to_char("Error: call to createStasisSphear in an invalid room.\nPlease report to an Admin.\n", ch);
    return;
  }

  for (t_char = world[ch->in_room].people; t_char; t_char = t_char->next_in_room) {
    GET_HIT(t_char) += GET_SKILL(ch, PSIONIC_STASIS_FIELD) / 10;
    if (GET_HIT(t_char) > GET_MAX_HIT(t_char))
      GET_HIT(t_char) = GET_MAX_HIT(t_char);
    update_pos(t_char);
  }

  if (IS_AFFECTED(ch, AFF_STASIS_FIELD))
    AddEvent(EVENT_CHAR_EXECUTE, 2 * PULSE_VIOLENCE, TRUE, ch, createStasisSphear);
  else {
    affect_from_char(ch, PSIONIC_STASIS_FIELD);
    send_to_char("&+WThe sphere around you slowly dissipates.\n", ch);
    act("The translucent sphere centered around $n slowly dissipates.", TRUE, ch, 0, 0, TO_ROOM);
  }
}

void do_ultrablast(struct char_data *ch, char *arg, int cmd) {
  P_char vict = NULL, next = NULL;
  int skl_lvl, dam, psp, num_mobs;
  int landed = 0, attempted = 0;

  if (!ch) {
    log("assert: bogus parm in do_ultrablast");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_ULTRABLAST);

  if (psp == 0) {
    send_to_char("You've heard about it, unfortunately you have no idea how to do that.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  if (CHAR_IN_SAFE_ZONE(ch)) {
    send_to_char("You feel ashamed trying to disrupt the tranquility of this place.\n", ch);
    return;
  }

  if ((IS_DARK(ch->in_room) || IS_CSET(world[ch->in_room].room_flags, MAGIC_DARK))
          && !CAN_SEE(ch, ch)) {
    send_to_char("You can't even see your nose in front of your face!\n", ch);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_ULTRABLAST);

  num_mobs = area_valid_targets(ch);

  if (IS_PC(ch))
    dam = dice(skl_lvl, 9) * DAMFACTOR;
  if (IS_NPC(ch))
    dam = dice((GET_LEVEL(ch) * (3 / 2)), 8) * DAMFACTOR;

  send_to_char("&+LYou feel a field of &N&+rlethal energy&N&+L start expanding outward.&N\n", ch);
  act("&+LYou suddenly feel a strong&N&+r energy shockwave!&N", FALSE, ch, 0, 0, TO_ROOM);
  act("&+Loriginating from $n!", TRUE, ch, 0, 0, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = next) {
    next = vict->next_in_room;
    if (AreaAffectCheck(ch, vict)) {
      dam = psiDamage(ch, vict, 9, PSIONIC_ULTRABLAST); /* fix later -Iyachtu */
      if (IS_PC(ch))
        dam += (dam / 20);
      else
        dam = (dam * 7 / 10);
      attempted++;
      if (AreaSave(ch, vict, &dam, num_mobs) == AVOID_AREA)
        continue;
      landed++;

      psiDamage(ch, vict, dam, PSIONIC_ULTRABLAST);

      debuglog(51, DS_D2, "%s ultrablast: target - %s, skill - %d, damage - %d",
              GET_NAME(ch), GET_NAME(vict), skl_lvl, dam);
      damage(ch, vict, dam, PSIONIC_ULTRABLAST);
    }
  }

  area_message(ch, landed, attempted - landed, "ultrablast");
  incSkillSubPsp(ch, PSIONIC_ULTRABLAST, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, 0, arg, PSIONIC_ULTRABLAST);

  return;
}

void do_battle_trance(struct char_data *ch, char *arg, int cmd) {

  P_char t_char = NULL, next = NULL;
  int skl_lvl, duration, modifier, allowBerserk = FALSE, psp;
  struct affected_type af;

  if (!ch) {
    log("assert: bogus parm in do_trance");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_BATTLE_TRANCE);

  if (psp == 0) {
    send_to_char("Battle Trance? Aye, you've heard about it before..\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_BATTLE_TRANCE);

  duration = MAX(4 * PULSE_VIOLENCE, ((skl_lvl / 20) * PULSE_VIOLENCE) * 4);
  modifier = MAX(1, (skl_lvl + 1) / 20);

  /* If the skill is high enough (plus a small chance) force em into Berserk */
  if ((skl_lvl > 80) && !number(0, 4))
    allowBerserk = number(1, 5);

  /* The modifier for dam/hit rolls is determined by skill_level / 20
   * yielding a range of 1 to 5
   * The duration ranges between 4 and 20 combat rounds and is a function of
   * the skill_level..
   */
  for (t_char = world[ch->in_room].people; t_char; t_char = next) {
    next = t_char->next_in_room;
    if (IS_GROUPED(ch, t_char) && IS_FIGHTING(t_char)
            && !affected_by_spell(t_char, PSIONIC_BATTLE_TRANCE)) {
      bzero(&af, sizeof (af));
      CLEAR_CBITS(af.sets_affs, AFF_BYTES);
      af.type = PSIONIC_BATTLE_TRANCE;
      af.duration = number(1, 2);
      af.modifier = modifier;
      af.location = APPLY_HITROLL;
      affect_to_char(t_char, &af);
      CLEAR_CBITS(af.sets_affs, AFF_BYTES);
      af.type = PSIONIC_BATTLE_TRANCE;
      af.duration = number(1, 2);
      af.modifier = modifier;
      af.location = APPLY_DAMROLL;
      affect_to_char(t_char, &af);

      act("$N's projection improves your fighting ability.", TRUE, t_char, 0, ch, TO_CHAR),
              act("Your projection improves $N fighting ability.", TRUE, ch, 0, t_char, TO_CHAR);

      if (!affected_by_spell(t_char, PSIONIC_BERSERK) && allowBerserk) {
        Berserk(t_char, TRUE, PSIONIC_BATTLE_TRANCE);
        act("You force $N into mindless battle rage!", TRUE, ch, 0, t_char, TO_CHAR);
        send_to_char("Your vision blurs as a wave of FURY overcomes you!\n", t_char);
        allowBerserk--;
      }
    }
  }

  incSkillSubPsp(ch, PSIONIC_BATTLE_TRANCE, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, 0, arg, PSIONIC_BATTLE_TRANCE);

  return;
}

void do_metaglobe(struct char_data *ch, char *arg, int cmd) {
  struct affected_type af;
  P_char victim;
  int skl_lvl, psp, bitmask = 0, lvl;
  char tbuf1[MAX_STRING_LENGTH];
  char mask[MAX_STRING_LENGTH];
  char *tptr;

  if (!ch) {
    log("assert: bogus param in do_metaglobe");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_METAGLOBE);

  if (psp == 0) {
    send_to_char("You don't know how to create major globes.\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  victim = ParseTarget(ch, arg);

  if (victim == ch->specials.fighting)
    victim = ch;

  if (IS_AFFECTED(victim, AFF_METAGLOBE)) {
    act("&+rThe shimmer around $n&N&+r intensifies for a moment.", TRUE, victim, 0, 0, TO_ROOM);
    act("&+rThe shimmer around you intensifies for a moment.", TRUE, victim, 0, 0, TO_CHAR);
    return;
  }

  half_chop(arg, tbuf1, mask);

  /* nuke everything past the second argument, 32 is space */
  tptr = mask;
  for (; (*tptr != 0) && (*tptr != 32); tptr++);
  *tptr = 0;

  /* check if all are digits and convert */
  skl_lvl = GET_SKILL(ch, PSIONIC_METAGLOBE);
  lvl = skl_lvl;
  tptr = mask;
  for (; *tptr != 0; tptr++) {
    if (!isdigit(*tptr)) {
      send_to_char("Invalid circle exclusion mask.\n", ch);
      return;
    }
    switch (*tptr) {
      case '1':
        if (lvl >= 1)
          SET_BIT(bitmask, BIT_1);
        break;
      case '2':
        if (lvl >= 2)
          SET_BIT(bitmask, BIT_2);
        break;
      case '3':
        if (lvl >= 3)
          SET_BIT(bitmask, BIT_3);
        break;
      case '4':
        if (lvl >= 4)
          SET_BIT(bitmask, BIT_4);
        break;
      case '5':
        if (lvl >= 5)
          SET_BIT(bitmask, BIT_5);
        break;
      case '6':
        if (lvl >= 6)
          SET_BIT(bitmask, BIT_6);
        break;
      case '7':
        if (lvl >= 7)
          SET_BIT(bitmask, BIT_7);
        break;
      case '8':
        if (lvl >= 8)
          SET_BIT(bitmask, BIT_8);
        break;
      case '9':
        if (lvl >= 9)
          SET_BIT(bitmask, BIT_9);
        break;
      case '0':
        if (lvl >= 0)
          SET_BIT(bitmask, BIT_10);
        break;
      default:
        dump_core();
    }
  }

  /* set the aff structure from which we extract the circle
   * exclusion mask.. */
  bzero(&af, sizeof (af));
  af.type = PSIONIC_METAGLOBE;
  af.duration = 9999;
  af.modifier = bitmask;
  SET_CBIT(af.sets_affs, AFF_METAGLOBE);
  affect_to_char(victim, &af);

  if (IS_PC(ch))
    AddEvent(EVENT_CHAR_EXECUTE, dice(12, GET_LEVEL(ch)) + skl_lvl, TRUE, victim, removeMetaGlobe);
  else if (IS_NPC(ch))
    AddEvent(EVENT_CHAR_EXECUTE, dice(12, GET_LEVEL(ch)) + (GET_LEVEL(ch) * 2), TRUE, victim, removeMetaGlobe);

  act("$n&N&+r begins to shimmer.", TRUE, victim, 0, 0, TO_ROOM);
  act("&+rYou begin to shimmer.", TRUE, victim, 0, 0, TO_CHAR);

  incSkillSubPsp(ch, PSIONIC_METAGLOBE, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, victim, arg, PSIONIC_METAGLOBE);

  return;
}

void removeMetaGlobe(struct char_data *ch) {

  if (!ch) {
    log("assert: bogus param in removeMetaGlobe");
    dump_core();
  }

  if (ch->in_room == NOWHERE)
    return;

  affect_from_char(ch, PSIONIC_METAGLOBE);

  return;
}

void do_globe_of_darkness(struct char_data *ch, char *arg, int cmd) {
  struct affected_type af;
  int psp, skl_lvl;

  if (!ch) {
    log("assert: bogus param in do_globe_of_darkness");
    dump_core();
  }

  psp = canUsePsionicSkill(ch, PSIONIC_DARKNESS);

  if (psp == 0) {
    send_to_char("Yeah, you'd need a light switch to do that...\n", ch);
    return;
  }

  if (psp == -1) {
    send_to_char("You do not have enough psp's to project that.\n", ch);
    return;
  }

  if (IS_AFFECTED(ch, AFF_GLOBE_OF_DARKNESS)) {
    send_to_char("&+LYou are already supporting a globe of darkness.", ch);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_DARKNESS);

  /* setting AFF_GLOBE_OF_DARKNESS will cause the char_to_room to
   * see if the room entered already has a DARK flag on.. if no,
   * make it DARK and shedule an eventGlobeOfDarkness */

  bzero(&af, sizeof (af));
  af.type = PSIONIC_DARKNESS;
  af.duration = skl_lvl / 10;
  SET_CBIT(af.sets_affs, AFF_GLOBE_OF_DARKNESS);
  affect_to_char(ch, &af);

  if (IS_SUNLIT(ch->in_room)) {
    act("&+LThe area centered around &+L$n grows dark as $e completes $m's projection.", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char("&+LThe area centered around you grows dark as you complete your projection.\n", ch);
    SET_CBIT(world[ch->in_room].room_flags, ROOM_GLOBE_OF_DARKNESS);
  } else {
    act("&+LThe area centered around &+L$n grows slightly darker as $e completes $m's projection.", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char("&+LThe area centered around you grows slightly darker as you complete your projection.\n", ch);
  }

  AddEvent(EVENT_ROOM_EXECUTE, 1, TRUE, ch, eventGlobeOfDarkness);

  incSkillSubPsp(ch, PSIONIC_DARKNESS, psp, PSIONIC_GAIN_ON);
  psiSkillUsageLogging(ch, ch, arg, PSIONIC_DARKNESS);
  return;
}

void eventGlobeOfDarkness(struct char_data *ch, P_room room) {
  int tmpRealRoom;

  if (!ch) {
    log("assert: bogus param in eventGlobeOfDarkness");
    dump_core();
  }

  if (ch->in_room == NOWHERE)
    return;

  if (room->number == NOWHERE)
    return;

  /* check if the char is still in the room passed..
   * YES -> check if still has the aff_globe_darkness flag
   *   NO  -> nuke the DARK flag
   *   YES -> shedule another event..
   * NO  -> nuke the DARK flag..  */

  tmpRealRoom = real_room(room->number);


  if ((ch->in_room != tmpRealRoom) || !IS_AFFECTED(ch, AFF_GLOBE_OF_DARKNESS)) {
    if (IS_CSET(room->room_flags, ROOM_GLOBE_OF_DARKNESS)) {
      send_to_room("&+WThe dark cloud dissipates and the immediate area becomes bright again.\n", tmpRealRoom);
      REMOVE_CBIT(room->room_flags, ROOM_GLOBE_OF_DARKNESS);
    }
    return;
  }

  if (!IS_CSET(world[ch->in_room].room_flags, ROOM_GLOBE_OF_DARKNESS) && IS_SUNLIT(ch->in_room)) {
    act("&+LThe light fades to gray...\n", TRUE, ch, 0, 0, TO_ROOM);
    SET_CBIT(world[ch->in_room].room_flags, ROOM_GLOBE_OF_DARKNESS);
  }

  AddEvent(EVENT_ROOM_EXECUTE, 4, TRUE, ch, eventGlobeOfDarkness);

  return;
}

void do_charge(struct char_data *ch, char *arg, int cmd) {
  char Gbuf[MAX_STRING_LENGTH];
  int psp;
  struct obj_data * crystal;

  if (!ch) {
    log("assert: bogus param in do_charge");
    dump_core();
  }

  if ((psp = canUsePsionicSkill(ch, PSIONIC_CHARGE)) == 0) {
    send_to_char("You charge off wildly, looking for greener pastures.\n", ch);
    return;
  }

  if (!*arg) {
    *Gbuf = 0;
    strcat(Gbuf, "&+BPSP Crystal Status:\n\n");
    listPSPCrystalCharges(ch, Gbuf);
    send_to_char(Gbuf, ch);
    return;
  }

  if (IS_AFFECTED(ch, AFF_CHARGING)) {
    send_to_char("You are already charging one crystal.\n", ch);
    return;
  }

  one_argument(arg, Gbuf);
  if (!(crystal = get_obj_in_list_vis(ch, Gbuf, ch->carrying))) {
    send_to_char("You can't find it!\n", ch);
    return;
  }

  if (crystal->type != ITEM_PSP_CRYSTAL) {
    send_to_char("You can only charge PSP crystals.\n", ch);
    return;
  }

  /* make it nuke AFF_CHARGING off all crystals */
  if (!IS_AFFECTED(ch, AFF_CHARGING))
    nukeAllPSPCrystalsExcept(ch, NULL, FALSE);

  if (IS_CSET(crystal->sets_affs, AFF_CHARGING)) {
    send_to_char("You are already charging that crystal!\n", ch);
    return;
  }

  if (crystal->value[PSP_CRYSTAL_MAX_CAPACITY] <= crystal->value[PSP_CRYSTAL_CURRENT_CAPACITY]) {
    send_to_char("That crystal is already fully charged.\n", ch);
    return;
  }

  act("You start charging $p.", FALSE, ch, crystal, 0, TO_CHAR);

  /* nuke the charging flag and psp's off all other crystals */
  nukeAllPSPCrystalsExcept(ch, crystal, TRUE);

  SET_CBIT(crystal->sets_affs, AFF_CHARGING);
  SET_CBIT(ch->specials.affects, AFF_CHARGING);

  AddEvent(EVENT_CHAR_EXECUTE, PULSE_VIOLENCE, TRUE, ch, chargePSPCrystal);

  psiSkillUsageLogging(ch, NULL, arg, PSIONIC_CHARGE);

  CharWait(ch, PULSE_VIOLENCE);

  return;
}

/* Crystal Charging event function */

void chargePSPCrystal(struct char_data *ch) {
  struct obj_data * crystal;
  int skl_lvl, inc;
  char Gbuf[MAX_STRING_LENGTH];

  if (!ch)
    dump_core();

  if (ch->in_room == NOWHERE)
    return;

  if (!IS_AFFECTED(ch, AFF_CHARGING)) {
    send_to_char("You stop charging your crystal.\n", ch);
    nukeAllPSPCrystalsExcept(ch, NULL, FALSE);
    return;
  }

  crystal = findPSPCrystal(ch);
  if (!crystal) {
    nukeAllPSPCrystalsExcept(ch, NULL, FALSE);
    return;
  }

  skl_lvl = GET_SKILL(ch, PSIONIC_CHARGE);

  inc = 1 + skl_lvl / 25;

  if (IS_AFFECTED(ch, AFF_MEDITATE))
    inc <<= 1;

  crystal->value[PSP_CRYSTAL_CURRENT_CAPACITY] += inc;
  sprintf(Gbuf, "&+BCurrent charge for&N [ $p ] - %d/%d psp's.",
          crystal->value[PSP_CRYSTAL_CURRENT_CAPACITY],
          crystal->value[PSP_CRYSTAL_MAX_CAPACITY]);
  act(Gbuf, FALSE, ch, crystal, 0, TO_CHAR);

  /* done charging? nuke the overflow and exit */
  if (crystal->value[PSP_CRYSTAL_MAX_CAPACITY] <= crystal->value[PSP_CRYSTAL_CURRENT_CAPACITY]) {
    crystal->value[PSP_CRYSTAL_CURRENT_CAPACITY] = crystal->value[PSP_CRYSTAL_MAX_CAPACITY];
    stopChargingPSPCrystal(ch, crystal);
    return;
  }

  AddEvent(EVENT_CHAR_EXECUTE, PULSE_VIOLENCE, TRUE, ch, chargePSPCrystal);

  incSkillSubPsp(ch, PSIONIC_CHARGE, 0, PSIONIC_GAIN_ON);

  return;
}

void nukeChargeEvent(struct char_data *ch) {
  P_event e1, e2;

  if (!ch)
    dump_core();

  LOOP_EVENTS(e1, ch->events) {
    if (e1->type == EVENT_CHAR_EXECUTE && e1->target.t_func == chargePSPCrystal) {
      if (e1 && (current_event != e1)) {
        e2 = current_event;
        current_event = e1;
        RemoveEvent();
        current_event = e2;
      }
      chargePSPCrystal(ch);
    }
  }

  return;
}

struct obj_data * findPSPCrystal(struct char_data *ch) {
  struct obj_data * i;

  if (!ch)
    dump_core();

  for (i = ch->carrying; i; i = i->next_content) {
    if (i->type != ITEM_PSP_CRYSTAL)
      continue;
    if (IS_CSET(i->sets_affs, AFF_CHARGING) || i->value[PSP_CRYSTAL_CURRENT_CAPACITY])
      return i;
  }

  return NULL;
}

void stopChargingPSPCrystal(struct char_data *ch, struct obj_data * crystal) {
  REMOVE_CBIT(crystal->sets_affs, AFF_CHARGING);
  REMOVE_CBIT(ch->specials.affects, AFF_CHARGING);

  return;
}

/* nuke the AFF_CHARGING off all crystals in inventory except except_obj is its
 * !NULL, if removePSPs is true then nuke all psps as well */
void nukeAllPSPCrystalsExcept(struct char_data *ch, struct obj_data * except_obj, bool removePSPs) {
  struct obj_data * i;

  if (!ch || !ch || (except_obj && except_obj->type != ITEM_PSP_CRYSTAL))
    dump_core();

  for (i = ch->carrying; i; i = i->next_content) {
    if (i->type == ITEM_PSP_CRYSTAL && i != except_obj) {
      if (removePSPs)
        i->value[PSP_CRYSTAL_CURRENT_CAPACITY] = 0;
      REMOVE_CBIT(i->sets_affs, AFF_CHARGING);
    }
  }

  return;
}

void listPSPCrystalCharges(struct char_data *ch, char *Gbuf) {
  struct obj_data * i;

  if (!Gbuf || !ch)
    dump_core();

  for (i = ch->carrying; i; i = i->next_content) {
    if (i->type == ITEM_PSP_CRYSTAL) {
      sprintf(Gbuf + strlen(Gbuf), "%-60s %4d/%4d\n",
              i->short_description,
              i->value[PSP_CRYSTAL_CURRENT_CAPACITY],
              i->value[PSP_CRYSTAL_MAX_CAPACITY]);
    }
  }

  return;
}

void do_amplify(struct char_data *ch, char *arg, int cmd) {
}

#define ENHANCE_STR     0
#define ENHANCE_AGI     1
#define ENHANCE_DEX     2
#define ENHANCE_CON     3
#define ENHANCE_VISION  4
#define ENHANCE_STAMINA 5

const char *enhance_keywords[] = {

  "strength",
  "agility",
  "dexterity",
  "constitution",
  "vision",
  "stamina",
  "\n"
};

const int enhance_values[] = {

  ENHANCE_STR,
  ENHANCE_AGI,
  ENHANCE_DEX,
  ENHANCE_CON,
  ENHANCE_VISION,
  ENHANCE_STAMINA,
  -1
};

#endif

/* the command parsing thing for enhance, based on do_world */

/*
void do_enhance (struct char_data *ch, char *arg, int cmd)
{
   struct affected_type af;
   P_char victim;
   int level, enhance_index;
   char first_arg[MAX_INPUT_LENGTH], tail_arg[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];

   if (!ch) {
      logit ( "assert: bogus param in do_enhance");
      dump_core();
   }

   if(IS_PC(ch)) {
      skl_lvl =  GET_SKILL(ch, PSIONIC_ENHANCE);
      if (!skl_lvl && !IS_TRUSTED(ch)) {
         send_to_char("You have no idea how to enhance yourself or anybody else.\n", ch);
         return;
      }
   }

   victim = ParseTarget(ch, arg);
   if (victim!=ch) {
      send_to_char("You can only enhance yourself.\n", ch);
      return;
   }


   if (!(!*arg || !arg)) {
      half_chop(arg, first_arg, tail_arg);
      enhance_index = search_block(first_arg, enhance_keywords, FALSE);
   }
   else {
    sprintf(buf, "You only know how to enhance:\n");
    if( GET_SKILL(ch, PSIONIC_ENHANCE) > 5)
       sprintf( buf, " Strength\n Agility\n Dexterity\n Constitution\n");
    if( GET_SKILL(ch, PSIONIC_ENHANCE) > 30)
       sprintf( buf, " Vision\n" );
    if( GET_SKILL(ch, PSIONIC_ENHANCE) > 50)
       sprintf( buf, " Stamina\n" );
    send_to_char( buf, ch );
    return;
   }

   switch(enhance_values[enhance_index]) {
    case ENHANCE_STR:
      break;
    case ENHANCE_AGI:
      break;
    case ENHANCE_DEX:
      break;
    case ENHANCE_CON:
      break;
    case ENHANCE_VISION:
      break;
    case ENHANCE_STAMINA:
      break;
   }

}



void psionic_enhance_str(struct char_data *ch)
{

  struct affected_type af;

  if (affected_by_spell(ch, PSIONIC_ENHANCE_STR)) {
    send_to_char("&+rExpanding your muscles any further would be decisively fatal.\n", ch);
    send_to_char("&+rYour manipulation has no effect.\n", ch);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = PSIONIC_ENHANCE_STR;
  af.duration = GET_LEVEL(ch);
  af.location = APPLY_STR;
  af.modifier = MAX(3,  GET_SKILL(ch, PSIONIC_ENHANCE)/10);
  affect_to_char(ch, &af);

  act("&+CA momentary wave of pain washes over your body as your muscles grow and reform.", TRUE, ch, 0, ch, TO_CHAR);
  act("&+C$N winces in pain as $S muscle tissue reforms slightly.", TRUE, ch, 0, ch, TO_NOTVICT);
  return;
}


void psionic_enhance_dex(struct char_data *ch)
{

  struct affected_type af;

  if (affected_by_spell(victim, PSIONIC_ENHANCE_DEX)) {
    send_to_char("&+rIncreasing your dexterity this way would most likely be fatal.&N\n", ch);
    send_to_char("&+rYour manipulation has no effect.&N\n", ch);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = PSIONIC_ENHANCE_DEX;
  af.duration = GET_LEVEL(ch);
  af.location = APPLY_DEX;
  af.modifier = MAX(3,  GET_SKILL(ch, PSIONIC_ENHANCE)/10);
  affect_to_char(ch, &af);

  act("&+CYou successfully increase your manual dexterity.", TRUE, ch, 0, ch, TO_CHAR);
  act("&+C$N hands shimmer slightly as $S projection takes effect.", TRUE, ch, 0, ch, TO_NOTVICT);
  return;
}

void psionic_enhance_agi(struct char_data *ch)
{

  struct affected_type af;

  if (affected_by_spell(victim, PSIONIC_ENHANCE_AGI)) {
    send_to_char("&+rYou can't get any more agile than this.\n", ch);
    send_to_char("&+rYour manipulation has no effect.\n", ch);
    return;
  }
  bzero(&af, sizeof(af));
  af.type = PSIONIC_ENHANCE_AGI;
  af.duration = GET_LEVEL(ch);
  af.location = APPLY_AGI;
  af.modifier = MAX(3,  GET_SKILL(ch, PSIONIC_ENHANCE)/10);
  affect_to_char(ch, &af);

  act("&+CYou suddenly feel your limbs move with greater ease and speed.", TRUE, ch, 0, ch, TO_CHAR);
  act("&+C$N shivers slightly as $S muscle composition changes.", TRUE, ch, 0, ch, TO_NOTVICT);
  return;
}

void do_enhance_con(struct char_data *ch)
{

  struct affected_type af;

  if (affected_by_spell(victim, PSIONIC_ENHANCE_CON)) {
    send_to_char("&+rYour constitution can't get any better that way.\n", ch);
    send_to_char("&+rYour manipulation has no effect.\n", ch);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = PSIONIC_ENHANCE_CON;
  af.duration = GET_LEVEL(ch);
  af.location = APPLY_CON;
  af.modifier = MAX(3,  GET_SKILL(ch, PSIONIC_ENHANCE)/10);
  affect_to_char(victim, &af);

  act("&+CYou successfully change your body structure.", TRUE, ch, 0, 0, TO_CHAR);
  act("&+C$N shivers as $S body structure changes and becomes bulkier.", TRUE, ch, 0, 0, TO_NOTVICT);
  return;

}



void psionic_enhance_vision(struct char_data *ch)
{

  struct affected_type af;

  if (affected_by_spell(ch, PSIONIC_ENHANCE_VISION) ||
       affected_by_spell(ch, SPELL_DETECT_INVISIBLE)) {
    send_to_char("&+rYour constitution can't get any better that way.\n", ch);
    send_to_char("&+rYour manipulation has no effect.\n", ch);
    return;
  }

  bzero(&af, sizeof(af));
  af.type = PSIONIC_ENHANCE_CON;
  af.duration = GET_LEVEL(ch);
  af.location = APPLY_CON;
  af.modifier = MAX(3,  GET_SKILL(ch, PSIONIC_ENHANCE)/10);
  affect_to_char(victim, &af);

  act("&+CYou successfully change your body structure.", TRUE, ch, 0, 0, TO_CHAR);
  act("&+C$N shivers as $S body structure changes and becomes bulkier.", TRUE, ch, 0, 0, TO_NOTVICT);
  return;

}
 */



/* yanked.. makes sense but would make the damn squids too powerful */

#if 0

void do_telekinate(struct char_data *ch, char *arg, int cmd) {

  P_char victim;
  int level;
  int delay;

  if (!(victim && ch)) {
    log("assert: bogus parms in spell_empathic_healing()");
    dump_core();
  }

  if (ch == victim) {
    send_to_char("Aren't we funny today. Something stops you from hurling yourself around.", ch);
    return;
  }

  /* Throw the victim down on its knees and Lag the caster for
   * a number of rounds depending on the victims power.. */

  delay = 1 + (level / 15);
  SET_POS(victim, POS_KNEELING + GET_STAT(victim));
  GET_HIT(victim) -= level;
  CharWait(victim, PULSE_VIOLENCE * delay);
  if (!IS_TRUSTED(ch))
    CharWait(ch, PULSE_VIOLENCE * delay * 2);

  act("&+c$n throws you upon your knees with a strong psionic blast.", TRUE, ch, 0, victim, TO_VICT);
  act("&+cYou successfully throw $N upon $S knees.", TRUE, ch, 0, victim, TO_CHAR);
  act("&+c$n throws $N upon $S knees with a strong psionic blast.", TRUE, ch, 0, victim, TO_ROOM);

  return;
}
#endif



/*
void spell_molecular_control(int level, struct char_data *ch, P_char victim, struct obj_data * obj)
{

  P_char victim;
  int level;
  struct affected_type af;

  if (IS_AFFECTED(ch, AFF_PASSDOOR))
    return;

  bzero(&af, sizeof(af));
  af.type = SPELL_MOLECULAR_CONTROL;
  af.duration = level / 4;
  SET_CBIT(af.sets_affs, AFF_PASSDOOR);
  affect_to_char(ch, &af);

  act("&+YYou gain a keener understanding of your molecules!", FALSE, ch,0, 0, TO_CHAR);
  act("&+W$n vibrates for a second, then is still.", TRUE, victim, 0, 0,TO_ROOM);

  return;
}
 */

#ifdef PSI_CLASS_ENABLED
#undef PSI_CLASS_ENABLED
#endif
