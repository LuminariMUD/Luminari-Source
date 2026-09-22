/**
 * @file combat_messages.c
 * Combat presentation - turning a resolved attack into the text a room reads.
 *
 * Split out of fight.c so combat resolution and combat wording have separate
 * blast radii. Everything here is downstream of the outcome: by the time a
 * function in this file runs, the hit, the damage, and the death have already
 * been decided.
 *
 * combat_messages.h documents the ownership, nullability, buffer-lifetime,
 * and thread-safety rules for the two entry points.
 */

#include "conf.h"
#include "core/sysdep.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/db.h"
#include "core/handler.h"
#include "core/constants.h"
#include "core/screen.h"
#include "magic/spells.h"
#include "spec/spec_dispatch.h"
#include "fight.h"
#include "assign_wpn_armor.h"
#include "projectiles.h"
#include "combat_messages.h"

/* toggle for debug mode
   true = annoying messages used for debugging
   false = normal gameplay */
#define DEBUGMODE FALSE

/* this function replaces the #w or #W with an appropriate weapon
   constant dependent on plural or not */
static char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
  static char buf[MEDIUM_STRING] = {'\0'};
  char *cp = buf;
  const char *const end = buf + sizeof(buf) - 1; /* keep a byte for the terminator */

  for (; *str; str++)
  {
    /* a trailing '#' is literal text; consuming it would step past the end */
    if (*str == '#' && *(str + 1) != '\0')
    {
      switch (*(++str))
      {
      case 'W':
        for (; *weapon_plural && cp < end; *(cp++) = *(weapon_plural++))
          ;
        break;
      case 'w':
        for (; *weapon_singular && cp < end; *(cp++) = *(weapon_singular++))
          ;
        break;
      default:
        if (cp < end)
          *(cp++) = '#';
        break;
      }
    }
    else if (cp < end)
      *(cp++) = *str;

    *cp = 0;
  } /* For */

  return (buf);
}

/* message for doing damage with a weapon */
void dam_message(int dam, struct char_data *ch, struct char_data *victim, int w_type,
                 int attack_type, struct obj_data *projectile)
{
  int msgnum = -1, hp = 0, pct = 0;
  const char *ranged_to_room;
  const char *ranged_to_char;
  const char *ranged_to_victim;

  /* The text below indexes attack_hit_text[] by w_type - TYPE_HIT, and that
     table has exactly NUM_ATTACK_TYPES entries. Refuse anything else here
     rather than trust every caller to have checked IS_WEAPON() first. */
  if (!IS_WEAPON(w_type))
  {
    log("SYSERR: dam_message: w_type %d outside the weapon range [%d, %d)", w_type,
        TOP_ATTACK_TYPES, BOT_WEAPON_TYPES);
    return;
  }

  hp = GET_HIT(victim);
  if (GET_HIT(victim) < 1)
    hp = 1;

  pct = 100 * dam / hp;

  if (dam && pct <= 0)
    pct = 1;

  if (affected_by_spell(ch, SKILL_DRHRT_CLAWS))
    w_type = TYPE_CLAW;

  static struct dam_weapon_type
  {
    const char *to_room;
    const char *to_char;
    const char *to_victim;
  } dam_weapons[] = {
      /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */
      {"\tn$n tries to #w \tn$N, but misses.\tn", /* 0: 0     */
       "You try to #w \tn$N, but miss.\tn", "\tn$n tries to #w you, but misses.\tn"},
      {"\tn$n \tYbarely grazes \tn$N \tYas $e #W $M.\tn", /* 1: dam <= 2% */
       "\tMYou barely graze \tn$N \tMas you #w $M.\tn",
       "\tn$n \tRbarely grazes you as $e #W you.\tn"},
      {"\tn$n \tYnicks \tn$N \tYas $e #W $M.\tn", /* 2: dam <= 4% */
       "\tMYou nick \tn$N \tMas you #w $M.\tn", "\tn$n \tRnicks you as $e #W you.\tn"},
      {"\tn$n \tYbarely #W \tn$N\tY.\tn", /* 3: dam <= 6%  */
       "\tMYou barely #w \tn$N\tM.\tn", "\tn$n \tRbarely #W you.\tn"},
      {"\tn$n \tY#W \tn$N\tY.\tn", /* 4: dam <= 8%  */
       "\tMYou #w \tn$N\tM.\tn", "\tn$n \tR#W you.\tn"},
      {"\tn$n \tY#W \tn$N \tYhard.\tn", /* 5: dam <= 11% */
       "\tMYou #w \tn$N \tMhard.\tn", "\tn$n \tR#W you hard.\tn"},
      {"\tn$n \tY#W \tn$N \tYvery hard.\tn", /* 6: dam <= 14%  */
       "\tMYou #w \tn$N \tMvery hard.\tn", "\tn$n \tR#W you very hard.\tn"},
      {"\tn$n \tY#W \tn$N \tYextremely hard.\tn", /* 7: dam <= 18%  */
       "\tMYou #w \tn$N \tMextremely hard.\tn", "\tn$n \tR#W you extremely hard.\tn"},
      {"\tn$n \tYinjures \tn$N \tYwith $s #w.\tn", /* 8: dam <= 22%  */
       "\tMYou injure \tn$N \tMwith your #w.\tn", "\tn$n \tRinjures you with $s #w.\tn"},
      {"\tn$n \tYwounds \tn$N \tYwith $s #w.\tn", /* 9: dam <= 27% */
       "\tMYou wound \tn$N \tMwith your #w.\tn", "\tn$n \tRwounds you with $s #w.\tn"},
      {"\tn$n \tYinjures \tn$N \tYharshly with $s #w.\tn", /* 10: dam <= 32%  */
       "\tMYou injure \tn$N \tMharshly with your #w.\tn",
       "\tn$n \tRinjures you harshly with $s #w.\tn"},
      {"\tn$n \tYseverely wounds \tn$N \tYwith $s #w.\tn", /* 11: dam <= 40% */
       "\tMYou severely wound \tn$N \tMwith your #w.\tn",
       "\tn$n \tRseverely wounds you with $s #w.\tn"},
      {"\tn$n \tYinflicts grave damage on \tn$N\tY with $s #w.\tn", /* 12: dam <= 50% */
       "\tMYou inflict grave damage on \tn$N \tMwith your #w.\tn",
       "\tn$n \tRinflicts grave damage on you with $s #w.\tn"},
      {"\tn$n \tYnearly kills \tn$N\tY with $s deadly #w!!\tn", /* (13): > 51   */
       "\tMYou nearly kill \tn$N \tMwith your deadly #w!!\tn",
       "\tn$n \tRnearly kills you with $s deadly #w!!\tn"}};

  static struct dam_ranged_weapon_type
  {
    const char *to_room;
    const char *to_char;
    const char *to_victim;
  } dam_ranged[] = {
      {"*WHOOSH* $n fires $p at $N but misses!", /* 0: 0     */
       "\ty*WHOOSH*\ty you fire \tn$p\ty at \tn$N \tybut \tYmiss!\tn",
       "*WHOOSH* $n fires $p at you but misses!"},
      {"*THWISH* $n fires $p at $N grazing $M.", /* 1: dam <= 2% */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]grazing $M.\tn",
       "]*THWISH* $n fires $p at you grazing you."},
      {"*THWISH* $n fires $p at $N nicking $M.", /* 2: dam <= 4% */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]nicking $M.\tn",
       "*THWISH* $n fires $p at you nicking you."},
      {"*THWISH* $n fires $p at $N *THUNK* barely damaging $M.", /* 3: dam <= 6%  */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THUNK* barely "
       "damaging $M.\tn",
       "*THWISH* $n fires $p at you *THUNK* barely damaging you."},
      {"*THWISH* $n fires $p at $N *THUNK* damaging $M.", /* 4: dam <= 8%  */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THUNK* damaging "
       "$M.",
       "*THWISH* $n fires $p at you *THUNK* damaging you."},
      {"*THWISH* $n fires $p at $N *THUNK* damaging $M moderately!", /* 5: dam <= 11% */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THUNK* damaging "
       "$M moderately!\tn",
       "*THWISH* $n fires $p at you *THUNK* damaging you moderately!"},
      {"*THWISH* $n fires $p at $N *THUNK* damaging $M badly!", /* 6: dam <= 14%  */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THUNK* damaging "
       "$M badly!\tn",
       "*THWISH* $n fires $p at you *THUNK* damaging you badly!"},
      {"*THWISH* $n fires $p at $N *THUNK* injuring $M harshly!", /* 7: dam <= 18%  */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THUNK* injuring "
       "$M harshly!\tn",
       "*THWISH* $n fires $p at you *THUNK* injuring you harshly!"},
      {"*THWISH* $n fires $p at $N *THWAK* severely injuring $M!", /* 8: dam <= 22%  */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THWAK* severely "
       "injuring $M!\tn",
       "*THWISH* $n fires $p at you *THWAK* severely injuring you!"},
      {"*THWISH* $n fires $p at $N *THWAK* causing serious wounds to $M!", /* 9: dam <= 27% */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THWAK* causing "
       "serious wounds to $M!\tn",
       "*THWISH* $n fires $p at you *THWAK* causing serious wounds to you!"},
      {"*THFFFT* $n fires $p at $N *THWAK* damaging $M gravely!", /* 10: dam <= 32%  */
       "\t[f500]*THFFFT*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THWAK* damaging "
       "$M gravely!\tn",
       "*THFFFT* $n fires $p at you *THWAK* damaging you gravely!"},
      {"*THFFFT* $n fires $p at $N *THWAK* severely wounding $M!", /* 11: dam <= 40% */
       "\t[f500]*THFFFT*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THWAK* severely "
       "wounding $M!\tn",
       "*THFFFT* $n fires $p at you *THWAK* severely wounding you!"},
      {"*THFFFT* $n fires $p at $N *THWAK* lethally wounding $M!", /* 12: dam <= 50% */
       "\t[f500]*THFFFT*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THWAK* lethally "
       "wounding $M!\tn",
       "*THFFFT* $n fires $p at you *THWAK* lethally wounding you!"},
      {"*THFFFT* $n fires $p at $N *THWAK* nearly killing $M!", /* (13): > 51   */
       "\t[f500]*THFFFT*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THWAK* nearly "
       "killing $M!\tn",
       "*THFFFT* $n fires $p at you *THWAK* nearly killing you!"}};

  w_type -= TYPE_HIT; /* Change to base of table with text */

  if (pct == 0)
    msgnum = 0;
  else if (pct <= 2)
    msgnum = 1;
  else if (pct <= 4)
    msgnum = 2;
  else if (pct <= 6)
    msgnum = 3;
  else if (pct <= 8)
    msgnum = 4;
  else if (pct <= 11)
    msgnum = 5;
  else if (pct <= 14)
    msgnum = 6;
  else if (pct <= 18)
    msgnum = 7;
  else if (pct <= 22)
    msgnum = 8;
  else if (pct <= 27)
    msgnum = 9;
  else if (pct <= 32)
    msgnum = 10;
  else if (pct <= 40)
    msgnum = 11;
  else if (pct <= 50)
    msgnum = 12;
  else
    msgnum = 13;

  /* ranged, not dead */
  if (is_ranged_weapon_attack(attack_type) && projectile && GET_POS(victim) > POS_DEAD)
  {
    ranged_to_room = dam_ranged[msgnum].to_room;
    ranged_to_char = dam_ranged[msgnum].to_char;
    ranged_to_victim = dam_ranged[msgnum].to_victim;
    if (is_thrown_attack(attack_type))
    {
      if (msgnum == 0)
      {
        ranged_to_room = "$n throws $p at $N but misses!";
        ranged_to_char = "You throw $p at $N but miss!";
        ranged_to_victim = "$n throws $p at you but misses!";
      }
      else
      {
        ranged_to_room = "$n throws $p at $N and strikes $M!";
        ranged_to_char = "You throw $p at $N and strike $M!";
        ranged_to_victim = "$n throws $p at you and strikes you!";
      }
    }

    /* damage message to room */
    /* as a temporary solution we are sending a funky signal (ACT_CONDENSE_VALUE) via the hide_invisible field
         for condensed combat mode handling -zusuk */
    act(ranged_to_room, ACT_CONDENSE_VALUE, ch, projectile, victim, TO_NOTVICT);

    /* damage message to damager */
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_CONDENSED) && CNDNSD(ch))
    {
      CNDNSD(ch)->num_times_attacking++;
      CNDNSD(ch)->num_times_hit_targets_ranged++;
    }
    else
    {
      act(ranged_to_char, FALSE, ch, projectile, victim, TO_CHAR);
      send_to_char(ch, CCNRM(ch, C_CMP));
    }

    /* damage message to damagee */
    if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_CONDENSED) && CNDNSD(victim))
    {
      CNDNSD(victim)->num_times_others_attack_you++;
      CNDNSD(victim)->num_times_hit_by_others_ranged++;
    }
    else
    {
      send_to_char(victim, CCRED(victim, C_CMP));
      act(ranged_to_victim, FALSE, ch, projectile, victim, TO_VICT | TO_SLEEP);
      send_to_char(victim, CCNRM(victim, C_CMP));
    }
  }

  /* non ranged, not dead */
  else if (GET_POS(victim) > POS_DEAD)
  {
    char *buf = NULL;

    /* damage message to observers (to room) */
    buf = replace_string(dam_weapons[msgnum].to_room, attack_hit_text[w_type].singular,
                         attack_hit_text[w_type].plural);
    GUI_CMBT_NOTVICT_OPEN(ch, victim);
    /* as a temporary solution we are sending a funky signal (ACT_CONDENSE_VALUE) via the hide_invisible field
         for condensed combat mode handling -zusuk */
    act(buf, ACT_CONDENSE_VALUE, ch, NULL, victim, TO_NOTVICT);
    GUI_CMBT_NOTVICT_CLOSE(ch, victim);

    /* damage message to damager (to_ch) */
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_CONDENSED) && CNDNSD(ch))
    {
      CNDNSD(ch)->num_times_attacking++;
      CNDNSD(ch)->num_times_hit_targets_melee++;
    }
    else
    {
      buf = replace_string(dam_weapons[msgnum].to_char, attack_hit_text[w_type].singular,
                           attack_hit_text[w_type].plural);
      GUI_CMBT_OPEN(ch);
      act(buf, FALSE, ch, NULL, victim, TO_CHAR);
      send_to_char(ch, CCNRM(ch, C_CMP));
      GUI_CMBT_CLOSE(ch);
    }

    /* damage message to damagee (to_vict) */
    if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_CONDENSED) && CNDNSD(victim))
    {
      CNDNSD(victim)->num_times_others_attack_you++;
      CNDNSD(victim)->num_times_hit_by_others_melee++;
    }
    else
    {
      buf = replace_string(dam_weapons[msgnum].to_victim, attack_hit_text[w_type].singular,
                           attack_hit_text[w_type].plural);
      GUI_CMBT_OPEN(victim);
      act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
      send_to_char(victim, CCNRM(victim, C_CMP));
      GUI_CMBT_CLOSE(victim);
    }
  }
  else
  {
    /* Victim is at POS_DEAD: nothing is rendered. dam_weapons[] has no death
       text; a killing blow's line comes from the skill-message path and the
       death notice from damage_with_projectile(). See combat_messages.h. */
  }
}

/*  message for doing damage with a spell or skill. Also used for weapon
 *  damage on miss and death blows. */
/* took out attacking-staff-messages -zusuk*/
/* this is so trelux's natural attack reflects an actual object */
#define TRELUX_CLAWS 800

int skill_message(int dam, struct char_data *ch, struct char_data *vict, int attacktype,
                  int dualing)
{
  return skill_message_with_projectile(dam, ch, vict, attacktype, dualing, NULL);
}

int skill_message_with_projectile(int dam, struct char_data *ch, struct char_data *vict,
                                  int attacktype, int dualing, struct obj_data *projectile)
{
  int i, j, nr, return_value = SKILL_MESSAGE_MISS_FAIL;
  struct message_type *msg;
  struct obj_data *opponent_weapon = GET_EQ(vict, WEAR_WIELD_1);
  struct obj_data *weap = GET_EQ(ch, WEAR_WIELD_1);
  struct obj_data *shield = NULL;
  /* Tracked separately from weap, which the ranged and shield paths overwrite. */
  struct obj_data *trelux_claws = NULL;
  bool is_ranged = FALSE;

  if (DEBUGMODE)
  {
    send_to_char(
        ch,
        "Debug - We are in skill_message(), dam %d, ch %s, vict %s, attacktype %d, dualing %d\r\n",
        dam, GET_NAME(ch), GET_NAME(vict), attacktype, dualing);
    send_to_char(
        vict,
        "Debug - We are in skill_message(), dam %d, ch %s, vict %s, attacktype %d, dualing %d\r\n",
        dam, GET_NAME(ch), GET_NAME(vict), attacktype, dualing);
  }

  /* attacker weapon */
  if (is_second_pair_attack(dualing))
    weap = get_wielded(ch, dualing); /* four arms: the lower-arm weapon */
  else if (GET_EQ(ch, WEAR_WIELD_2H))
    weap = GET_EQ(ch, WEAR_WIELD_2H);
  else if (dualing == 1)
    weap = GET_EQ(ch, WEAR_WIELD_OFFHAND);

  /* special handling for Trelux: a stand-in object gives act() a $p to name
     their natural attack. It is never handed to anyone and is extracted at the
     single exit below. */
  if (GET_RACE(ch) == RACE_TRELUX)
  {
    trelux_claws = read_object(TRELUX_CLAWS, VIRTUAL);
    weap = trelux_claws;
    attacktype = TYPE_CLAW;
  }

  if (affected_by_spell(ch, SKILL_DRHRT_CLAWS))
    attacktype = TYPE_CLAW;

  /* ranged weapon - general check and we want the missile to serve as our weapon */
  if (projectile && is_ranged_weapon_attack(dualing))
  {
    is_ranged = TRUE;
    weap = projectile;
  }

  /* defender weapon for parry message */
  if (!opponent_weapon)
  {
    opponent_weapon = GET_EQ(vict, WEAR_WIELD_2H);
  }

  if (!opponent_weapon)
  { /* maybe no weapon in main hand, but offhand has one */
    opponent_weapon = GET_EQ(vict, WEAR_WIELD_OFFHAND);
  }

  if (GET_EQ(vict, WEAR_WIELD_1) && GET_EQ(vict, WEAR_WIELD_OFFHAND))
  {
    if (rand_number(0, 1))
      opponent_weapon = GET_EQ(vict, WEAR_WIELD_1);
    else
      opponent_weapon = GET_EQ(vict, WEAR_WIELD_OFFHAND);
  }

  /* These attacks use a shield as a weapon. */
  if ((attacktype == SKILL_SHIELD_PUNCH) || (attacktype == SKILL_SHIELD_CHARGE) ||
      (attacktype == SKILL_SHIELD_SLAM))
    weap = GET_EQ(ch, WEAR_SHIELD);

  for (i = 0; i < MAX_MESSAGES; i++)
  {
    /* first search through our messages trying to match the attacktype */
    if (fight_messages[i].a_type == attacktype)
    {
      /* might have several messages for that attacktype, pick a random one */
      nr = dice(1, fight_messages[i].number_of_attacks);
      /* increment the messages until we get to that selected message */
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
        msg = msg->next;
      /* an attack type listed without any messages has nothing to show */
      if (msg == NULL)
        continue;
      /* we now have a message! */

      /* old location of staff-messages */

      /* we did some damage or deathblow */
      if (dam != 0)
      {
        if (GET_POS(vict) == POS_DEAD)
        {
          /* death messages */

          /* Don't send redundant color codes for TYPE_SUFFERING & other types
           * of damage without attacker_msg. */

          if (is_ranged)
          {
            /* ranged attack death blow */
            if (is_thrown_attack(dualing))
            {
              act("$n throws $p at $N, and $E \tRcollapses\tn to the ground!", FALSE, ch, weap,
                  vict, TO_NOTVICT);
              act("You throw $p at $N, and $E \tRcollapses\tn to the ground!", FALSE, ch, weap,
                  vict, TO_CHAR);
              act("$n throws $p at you, and you \tRcollapse\tn to the ground!", FALSE, ch, weap,
                  vict, TO_VICT | TO_SLEEP);
            }
            else
            {
              act("* THWISH * $n fires $p at $N * THUNK * $E \tRcollapses\tn to the ground!", FALSE,
                  ch, weap, vict, TO_NOTVICT);
              act("* THWISH * you fire $p at $N * THUNK * $E \tRcollapses\tn to the ground!", FALSE,
                  ch, weap, vict, TO_CHAR);
              act("* THWISH * $n fires $p at you * THUNK * you \tRcollapse\tn to the ground!",
                  FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
            }

            return_value = SKILL_MESSAGE_DEATH_BLOW; /* no reason to stay here */
            goto release_claws;
          }
          else
          {
            /* NOT ranged death blow */
            if (msg->die_msg.attacker_msg)
            {
              send_to_char(ch, CCYEL(ch, C_CMP));
              act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
              send_to_char(ch, CCNRM(ch, C_CMP));
            }

            send_to_char(vict, CCRED(vict, C_CMP));
            act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
            send_to_char(vict, CCNRM(vict, C_CMP));

            act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);

            return_value = SKILL_MESSAGE_DEATH_BLOW;
            goto release_claws;
          }
        }
        else
        {
          /* we did some damage, but not dead */

          if (msg->hit_msg.attacker_msg && ch != vict)
          {
            if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_CONDENSED) && CNDNSD(ch))
            {
              CNDNSD(ch)->num_times_attacking++;
              CNDNSD(ch)->num_times_hit_targets++;
            }
            else
            {
              send_to_char(ch, CCYEL(ch, C_CMP));
              act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
              send_to_char(ch, CCNRM(ch, C_CMP));
            }
          }

          if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_CONDENSED) && CNDNSD(vict))
          {
            CNDNSD(vict)->num_times_others_attack_you++;
            CNDNSD(vict)->num_times_hit_by_others++;
          }
          else
          {
            send_to_char(vict, CCRED(vict, C_CMP));
            act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
            send_to_char(vict, CCNRM(vict, C_CMP));
          }

          /* as a temporary solution we are sending a funky signal (ACT_CONDENSE_VALUE) via the hide_invisible field
               for condensed combat mode handling -zusuk */
          act(msg->hit_msg.room_msg, ACT_CONDENSE_VALUE, ch, weap, vict, TO_NOTVICT);

          return_value = SKILL_MESSAGE_GENERIC_HIT;
          goto release_claws;
        } /* end 'did some damage but not dead' section */

      } /* end if-check for situation where we did some damage */
      else if (ch != vict)
      {
        /* dam == 0, we did not do any damage! */

        if (DEBUGMODE)
        {
          send_to_char(ch,
                       "Debug - We are in skill_message() - ZERO DAMAGE, dam %d, ch %s, vict %s, "
                       "attacktype %d, dualing %d\r\n",
                       dam, GET_NAME(ch), GET_NAME(vict), attacktype, dualing);
          send_to_char(vict,
                       "Debug - We are in skill_message() - ZERO DAMAGE, dam %d, ch %s, vict %s, "
                       "attacktype %d, dualing %d\r\n",
                       dam, GET_NAME(ch), GET_NAME(vict), attacktype, dualing);
        }

        /* do we have armor that can stop a blow? */
        struct obj_data *armor = GET_EQ(vict, WEAR_BODY);

        /* insert more colorful defensive messages here */

        /* shield block */
        if ((shield = GET_EQ(vict, WEAR_SHIELD)) && !rand_number(0, 3))
        {
          return_value = SKILL_MESSAGE_MISS_SHIELDBLOCK;

          if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_CONDENSED) && CNDNSD(ch))
          {
            CNDNSD(ch)->num_times_attacking++;
          }
          else
          {
            send_to_char(ch, CCYEL(ch, C_CMP));
            act("$N blocks your attack with $p!", FALSE, ch, shield, vict, TO_CHAR);
            send_to_char(ch, CCNRM(ch, C_CMP));
          }

          if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_CONDENSED) && CNDNSD(vict))
          {
            CNDNSD(vict)->num_times_others_attack_you++;
            CNDNSD(vict)->num_times_shieldblock++;
          }
          else
          {
            send_to_char(vict, CCRED(vict, C_CMP));
            act("You block $n's attack with $p!", FALSE, ch, shield, vict, TO_VICT | TO_SLEEP);
            send_to_char(vict, CCNRM(vict, C_CMP));
          }

          /* as a temporary solution we are sending a funky signal (ACT_CONDENSE_VALUE) via the hide_invisible field
               for condensed combat mode handling -zusuk */
          act("$N blocks $n's attack with $p!", ACT_CONDENSE_VALUE, ch, shield, vict, TO_NOTVICT);

          /* fire any shieldblock specs we might have */
          spec_gateway_defense_reaction(vict, shield, ch, "shieldblock");

          /* parry */
        }
        else if (opponent_weapon && !rand_number(0, 2))
        {
          return_value = SKILL_MESSAGE_MISS_PARRY;

          if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_CONDENSED) && CNDNSD(ch))
          {
            CNDNSD(ch)->num_times_attacking++;
          }
          else
          {
            send_to_char(ch, CCYEL(ch, C_CMP));
            act("$N parries your attack with $p!", FALSE, ch, opponent_weapon, vict, TO_CHAR);
            send_to_char(ch, CCNRM(ch, C_CMP));
          }

          if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_CONDENSED) && CNDNSD(vict))
          {
            CNDNSD(vict)->num_times_others_attack_you++;
            CNDNSD(vict)->num_times_parry++;
          }
          else
          {
            send_to_char(vict, CCRED(vict, C_CMP));
            act("You parry $n's attack with $p!", FALSE, ch, opponent_weapon, vict,
                TO_VICT | TO_SLEEP);
            send_to_char(vict, CCNRM(vict, C_CMP));
          }

          /* as a temporary solution we are sending a funky signal (ACT_CONDENSE_VALUE) via the hide_invisible field
               for condensed combat mode handling -zusuk */
          act("$N parries $n's attack with $p!", ACT_CONDENSE_VALUE, ch, opponent_weapon, vict,
              TO_NOTVICT);

          /* fire any parry specs we might have */
          spec_gateway_defense_reaction(vict, opponent_weapon, ch, "parry");

          /* glance off armor */
        }
        /* value[1] is only an armor_list[] index on an ITEM_ARMOR, but any item
           type can carry the BODY wear flag; GET_ARMOR_TYPE_PROF() checks. */
        else if (armor && GET_ARMOR_TYPE_PROF(armor) > ARMOR_TYPE_NONE && !rand_number(0, 2))
        {
          return_value = SKILL_MESSAGE_MISS_GLANCE;

          if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_CONDENSED) && CNDNSD(ch))
          {
            CNDNSD(ch)->num_times_attacking++;
          }
          else
          {
            send_to_char(ch, CCYEL(ch, C_CMP));
            act("Your attack glances off $p, protecting $N!", FALSE, ch, armor, vict, TO_CHAR);
            send_to_char(ch, CCNRM(ch, C_CMP));
          }

          if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_CONDENSED) && CNDNSD(vict))
          {
            CNDNSD(vict)->num_times_others_attack_you++;
            CNDNSD(vict)->num_times_glance++;
          }
          else
          {
            send_to_char(vict, CCRED(vict, C_CMP));
            act("$n's attack glances off $p!", FALSE, ch, armor, vict, TO_VICT | TO_SLEEP);
            send_to_char(vict, CCNRM(vict, C_CMP));
          }

          /* as a temporary solution we are sending a funky signal (ACT_CONDENSE_VALUE) via the hide_invisible field
               for condensed combat mode handling -zusuk */
          act("$n's attack glances off $p, protecting $N!", ACT_CONDENSE_VALUE, ch, armor, vict,
              TO_NOTVICT);

          /* fire any glance specs we might have */
          spec_gateway_defense_reaction(vict, armor, ch, "glance");
        }
        else
        {
          /* we fell through to generic miss message from file */

          return_value = SKILL_MESSAGE_MISS_GENERIC;

          /* default to miss messages in-file */
          if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_CONDENSED) && CNDNSD(ch))
          {
            CNDNSD(ch)->num_times_attacking++;
          }
          else
          {
            if (msg->miss_msg.attacker_msg)
            {
              send_to_char(ch, CCYEL(ch, C_CMP));
              act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
              send_to_char(ch, CCNRM(ch, C_CMP));
            }
          }

          if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_CONDENSED) && CNDNSD(vict))
          {
            CNDNSD(vict)->num_times_others_attack_you++;
            CNDNSD(vict)->num_times_dodge++;
          }
          else
          {
            send_to_char(vict, CCRED(vict, C_CMP));
            act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
            send_to_char(vict, CCNRM(vict, C_CMP));
          }

          /* as a temporary solution we are sending a funky signal (ACT_CONDENSE_VALUE) via the hide_invisible field
               for condensed combat mode handling -zusuk */
          act(msg->miss_msg.room_msg, ACT_CONDENSE_VALUE, ch, weap, vict, TO_NOTVICT);

          /* fire any dodge specs we might have, right now its only on weapons */
          if (opponent_weapon)
          {
            spec_gateway_defense_reaction(vict, opponent_weapon, ch, "dodge");
          }
        }
      } /* this ends our check for a scenario where no damage is inflicted */

      goto release_claws;
    } /* attacktype check */
  } /* for loop for damage messages */

  /* did not find a message to use? */

release_claws:
  if (trelux_claws != NULL)
    extract_obj(trelux_claws);

  return (return_value);
}

#undef TRELUX_CLAWS
#undef DEBUGMODE
