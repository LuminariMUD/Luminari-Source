/**************************************************************************
 *  File: spec/spec_mobile_archetypes.c                Part of LuminariMUD *
 *  Usage: Reusable legacy combat and companion mobile archetypes.         *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "magic/spells.h"
#include "constants.h"
#include "act.h"
#include "spec_mobile_archetypes.h"
#include "character/class.h"
#include "combat/fight.h"
#include "modify.h"
#include "obj/house.h"
#include "clan.h"
#include "mudlim.h"
#include "graph.h"
#include "dgscript/dg_scripts.h"
#include "mud_event.h"
#include "actions.h"
#include "combat/assign_wpn_armor.h"
#include "magic/domains_schools.h"
#include "character/feats.h"
#include "magic/spell_prep.h"
#include "obj/item.h"
#include "craft/alchemy.h"
#include "obj/treasure.h"
#include "mob/mob_utils.h"
#include "character/evolutions.h"
#include "olc/oasis.h"
#include "quest/quest.h"
#include "character/backgrounds.h"
#include "character/perks.h"

/*************************************************/
/**** General combat and companion archetypes. ***/
/*************************************************/

/* this is the generic dracolich procs -zusuk */
SPECIAL(dracolich_mob)
{
  struct char_data *vict = NULL;
  int hitpoints = 0, use_aoe = 0;

  if (!ch)
    return 0;

  /* note that the !vict is moved below */
  if (cmd)
    return 0;

  /* this is the offensive arsenal */
  if (FIGHTING(ch) && rand_number(0, 1))
  {
    if (!rand_number(0, 3) &&
        call_magic(ch, FIGHTING(ch), 0, SPELL_ACID_BREATHE, 0, GET_LEVEL(ch), CAST_INNATE))
    {
      /* looks like the breathe weapon worked */
      return 1;
    }
    else if (!rand_number(0, 3) && perform_tailsweep(ch))
    {
      /* looks like we did the tailsweeep successffully to at least one victim */
      return 1;
    }
    else if (!rand_number(0, 3) && perform_dragonfear(ch))
    {
      /* looks like we did the dragonfear to at least one victim */
      return 1;
    }
    else if (!rand_number(0, 4))
    {
      int i = 0;

      act("\tWWith power and determination you unleash an aggressive flurry of attacks!\tn", TRUE,
          ch, 0, FIGHTING(ch), TO_CHAR);
      act("$n\tL, with power and determination, unleashes an aggressive flurry of attacks!\tn",
          FALSE, ch, 0, FIGHTING(ch), TO_VICT);
      act("$n\tL, with power and determination, unleashes an aggressive flurry of attacks!\tn",
          TRUE, ch, 0, FIGHTING(ch), TO_NOTVICT);

      /* spam some attacks */
      for (i = 0; i <= rand_number(2, 4); i++)
      {
        if (valid_fight_cond(ch, TRUE))
          hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }
      return 1;
    }
  }
  /* special dracolich drain */
  else if (!rand_number(0, 6))
  {
    /* find random target, and num targets */
    if (!(vict = npc_find_target(ch, &use_aoe)))
      return 0;

    act("\tWWith a grin, you whisper, 'die' while touching $N, who keels over and falls over in "
        "excruciating pain!\tn",
        TRUE, ch, 0, vict, TO_CHAR);

    act("\tL$n cackles with glee at the fray, enjoying every second of the battle\r\n"
        "\tL $s sets her gaze upon you with the most wicked grin you have ever known.",
        FALSE, ch, 0, vict, TO_VICT);
    act("\tWAAAHHHH! You SCREAM in agony, a pain more intense than you have ever felt!\r\n"
        "\tWAs you flail in pain, you see a stream of your own life force flowing away from you..",
        FALSE, ch, 0, vict, TO_VICT);
    act("\tLAs the life drains from your body, you see $n's wicked grin staring into your "
        "soul..\tn",
        FALSE, ch, 0, vict, TO_VICT);

    act("$n \tLturns and gazes at \tn$N\tL, who freezes in place.\tn\r\n"
        "$n \tLreaches out with a skeletal claw and touches \tn$N\tL!\tn",
        TRUE, ch, 0, vict, TO_NOTVICT);
    act("\tL$N\tr SCREAMS\tL in agony, doubling over in pain so intense it makes you "
        "cringe!!\tn\r\n"
        "$n\tL literally sucks the life force from $N,\tn\r\n"
        "\tLwho crumples into a ball of unfathomable pain onto the ground...\tn",
        TRUE, ch, 0, vict, TO_NOTVICT);

    /* added a way to reduce the effectiveness of this attack -zusuk */
    if (AFF_FLAGGED(vict, AFF_DEATH_WARD) && !rand_number(0, 2))
    {
      hitpoints = damage(ch, vict, rand_number(100, MAX(100, GET_LEVEL(ch) * 20)), -1, DAM_UNHOLY,
                         FALSE); // type -1 = no dam message
    }
    else
    {
      if (GET_HIT(vict) <= 20)
      { /* try to finish the victim */
        hitpoints = damage(ch, vict, rand_number(100, MAX(100, GET_LEVEL(ch) * 20)), -1, DAM_UNHOLY,
                           FALSE); // type -1 = no dam message
      }
      else
      {
        hitpoints = GET_HIT(vict);
        GET_HIT(vict) = 21;
      }
    }

    /* heal/vamp effect from the attack */
    if (hitpoints < 120)
      hitpoints = 120;

    GET_HIT(ch) += hitpoints;

    return 1;
  }

  return 0;
}

/* custom mob code for vampire mobs -zusuk */
SPECIAL(vampire_mob)
{
  if (cmd)
    return 0;

  if (!ch)
    return 0;

  int rejuv = 0;
  struct char_data *vict = FIGHTING(ch);
  struct obj_data *corpse = NULL;

  /* this is the vampire's defensive arsenal */
  if (!rand_number(0, 6) && GET_HIT(ch) < GET_MAX_HIT(ch))
  {
    rejuv = GET_HIT(ch) + dice(10, GET_LEVEL(ch));

    if (rejuv > GET_MAX_HIT(ch))
      rejuv = GET_MAX_HIT(ch);

    GET_HIT(ch) = rejuv;

    if (vict) /* flavor messages */
    {
      if (rand_number(0, 1))
        act("\tr$n turns into gaseous form to escape the fray then turns back into vampire "
            "form!\tn",
            FALSE, ch, 0, 0, TO_ROOM);
      else
        act("\tr$n turns into a bat to escape the fray then turns back into vampire form!\tn",
            FALSE, ch, 0, 0, TO_ROOM);
    }

    act("\trThe wounds on $n's body begin to close as $e is regenerated!\tn", FALSE, ch, 0, 0,
        TO_ROOM);

    if (vict) /* flavor messages */
    {
      act("\tr$n returns to the fray!\tn", FALSE, ch, 0, 0, TO_ROOM);
    }

    /* removed the call to return here to make sure we can process an offensive proc */
    // return 1;
  }

  /* this is the vampire's regular form offensive arsenal */
  if (vict)
  {
    /* make sure we have our followers! */
    if (!PROC_FIRED(ch))
    {
      /* set up a group if we don't have one */
      if (!GROUP(ch))
      {
        create_group(ch);
      }

      /* get our children of the night first! */
      act("You reach out into the wilds to pull forth your children of the night.", FALSE, ch, 0, 0,
          TO_CHAR);
      act("$n reaches out into the wilds to pull forth children of the night.", FALSE, ch, 0, 0,
          TO_ROOM);
      call_magic(ch, ch, 0, VAMPIRE_ABILITY_CHILDREN_OF_THE_NIGHT, 0, GET_LEVEL(ch), CAST_INNATE);

      /* now create our vampire spawn */
      act("You turn to a nearby minion, grab him by the neck, and with a smile snap his neck.",
          FALSE, ch, 0, 0, TO_CHAR);
      act("$n turns to a nearby minion, grabs him by the neck, and with a smile snaps his neck.  "
          "The fresh corpse conveniently lays before $n.",
          FALSE, ch, 0, 0, TO_ROOM);

      /* this creates a generic corpse */
      corpse = make_a_corpse_4_npcs(ch);
      if (corpse)
      {
        /* messaging and actual call fo spell if we got a corpse */
        act("You draw upon your vampiric strength and attempt to convert $p into vampiric spawn",
            FALSE, ch, corpse, 0, TO_CHAR);
        act("$n draws upon vampiric strength and attempts to convert $p into vampiric spawn", FALSE,
            ch, corpse, 0, TO_ROOM);
        call_magic(ch, ch, corpse, ABILITY_CREATE_VAMPIRE_SPAWN, 0, GET_LEVEL(ch), CAST_INNATE);
      }

      /* done */
      PROC_FIRED(ch) = TRUE;
    }

    /* vampire bite */
    if (!rand_number(0, 3))
    {
      act("$n sinks $s fangs into $N!", 1, ch, 0, vict, TO_NOTVICT);
      act("$n sinks $s fangs into you!", 1, ch, 0, vict, TO_VICT);
      call_magic(ch, vict, 0, SPELL_POISON, 0, GET_LEVEL(ch), CAST_INNATE);
      damage(ch, vict, rand_number(6, MAX(6, GET_LEVEL(ch))), -1, DAM_POISON, FALSE);

      return 1;
    }
    /* blood drain */
    else if (!rand_number(0, 3))
    {
      act("You quickly pin $N.", FALSE, ch, 0, vict, TO_CHAR);
      act("$n briefly pins $N!", 1, ch, 0, vict, TO_NOTVICT);
      act("$n briefly pins you!", 1, ch, 0, vict, TO_VICT);
      vamp_blood_drain(ch, vict);

      return 1;
    }
    /* vicious attacks */
    else if (!rand_number(0, 3))
    {
      int i = 0;

      act("$n acts with inhuman speed!", 1, ch, 0, NULL, TO_ROOM);

      /* spam some attacks */
      for (i = 0; i <= rand_number(3, 6); i++)
      {
        if (valid_fight_cond(ch, TRUE))
          hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }

      return 1;
    }
  }

  return 0;
}

SPECIAL(cityguard)
{
  struct char_data *tch, *evil, *spittle;
  int max_evil, min_cha;

  if (cmd || !AWAKE(ch) || FIGHTING(ch))
    return (FALSE);

  max_evil = 1000;
  min_cha = 6;
  spittle = evil = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (!CAN_SEE(ch, tch))
      continue;
    if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_KILLER))
    {
      act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0,
          TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      return (TRUE);
    }

    if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_THIEF))
    {
      act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0,
          TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      return (TRUE);
    }

    if (FIGHTING(tch) && GET_ALIGNMENT(tch) < max_evil && (IS_NPC(tch) || IS_NPC(FIGHTING(tch))))
    {
      max_evil = GET_ALIGNMENT(tch);
      evil = tch;
    }

    if (GET_CHA(tch) < min_cha)
    {
      spittle = tch;
      min_cha = GET_CHA(tch);
    }
  }

  /*
  if (evil && GET_ALIGNMENT(FIGHTING(evil)) >= 0) {
    act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
    hit(ch, evil, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    return (TRUE);
  }
   */

  /* Reward the socially inept. */
  if (spittle && !rand_number(0, 9))
  {
    static int spit_social;

    if (!spit_social)
      spit_social = find_command("spit");

    if (spit_social > 0)
    {
      char spitbuf[MAX_NAME_LENGTH + 1];
      strncpy(spitbuf, GET_NAME(spittle), sizeof(spitbuf)); /* strncpy: OK */
      spitbuf[sizeof(spitbuf) - 1] = '\0';
      do_action(ch, spitbuf, spit_social, 0);
      return (TRUE);
    }
  }
  return (FALSE);
}

/* from Homeland */
// doesnt work properly if multiple instances.. :) -V

SPECIAL(practice_dummy)
{
  int rounddam = 0;
  static int round_count;
  static int max_hit;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  if (cmd)
    return FALSE;

  if (!FIGHTING(ch))
  {
    GET_MAX_HIT(ch) = 20000;
    GET_HIT(ch) = 20000;
    max_hit = 0;
    round_count = 0;
  }
  else
  {
    rounddam = GET_MAX_HIT(ch) - GET_HIT(ch);
    max_hit += rounddam;
    round_count++;

    snprintf(buf, sizeof(buf), "\tP%d damage last round!\tn  \tc(total: %d rounds: %d)\tn\r\n",
             rounddam, max_hit, round_count);
    send_to_room(ch->in_room, "%s", buf);
    GET_HIT(ch) = GET_MAX_HIT(ch);
    return TRUE;
  }
  return FALSE;
}

/* from Homeland */
SPECIAL(wraith)
{
  if (cmd)
    return FALSE;

  if (GET_POS(ch) == POS_DEAD || !ch->master)
  {
    act("With a loud shriek, $n crumbles into dust.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (ch->master && ch->in_room == ch->master->in_room)
    if (FIGHTING(ch->master) && rand_number(0, 1))
    {
      perform_assist(ch, ch->master);
      return TRUE;
    }

  return FALSE;
}

/* from Homeland */
SPECIAL(skeleton_zombie)
{
  if (cmd)
    return FALSE;

  if (GET_POS(ch) == POS_DEAD || !ch->master)
  {
    act("With a loud shriek, $n crumbles into dust.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (ch->master && ch->in_room == ch->master->in_room)
    if (FIGHTING(ch->master) && !rand_number(0, 2))
    {
      perform_assist(ch, ch->master);
      return TRUE;
    }

  return FALSE;
}

/* from Homeland */
SPECIAL(vampire)
{
  struct char_data *vict;

  if (cmd)
    return FALSE;

  if (GET_POS(ch) == POS_DEAD || !ch->master)
  {
    act("With a loud shriek, $n crumbles into dust.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (ch->master && ch->in_room == ch->master->in_room)
  {
    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    {
      if (FIGHTING(vict) == ch->master && !rand_number(0, 1))
      {
        perform_rescue(ch, ch->master);
        return TRUE;
      }
    }
  }

  return FALSE;
}

/* from Homeland */
SPECIAL(totemanimal)
{
  if (cmd)
    return FALSE;
  if (!ch->master)
    return FALSE;

  if (ch->master && ch->in_room == ch->master->in_room)
    if (FIGHTING(ch->master))
      perform_assist(ch, ch->master);
  return FALSE;
}

/* from Homeland */
SPECIAL(shades)
{
  if (cmd)
    return FALSE;

  if (GET_MAX_HIT(ch) > 1 && GET_HIT(ch) > 1)
  {
    GET_MAX_HIT(ch) = 1;
    GET_HIT(ch) = 1;
  }

  if (GET_POS(ch) == POS_DEAD)
    return FALSE;
  if (GET_HIT(ch) < GET_MAX_HIT(ch) || !ch->master)
  {
    act("A shade evaporates into thin air.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (ch->in_room != ch->master->in_room)
  {
    HUNTING(ch) = ch->master;
    hunt_victim(ch);
    return TRUE;
  }
  return FALSE;
}

/* from Homeland */
SPECIAL(solid_elemental)
{
  struct char_data *vict;

  if (cmd)
    return FALSE;

  if (GET_POS(ch) == POS_DEAD || (!ch->master && !MOB_FLAGGED(ch, MOB_MEMORY)))
  {
    act("With a loud shriek, $n returns to $s home plane.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (GET_HIT(ch) > 0)
  {
    if (ch->master && ch->in_room == ch->master->in_room && !rand_number(0, 1))
    {
      for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
      {
        if (FIGHTING(vict) == ch->master)
        {
          perform_rescue(ch, ch->master);
          return TRUE;
        }
      }
    }

    if (!FIGHTING(ch) && ch->master && FIGHTING(ch->master) && ch->in_room == ch->master->in_room)
    {
      perform_assist(ch, ch->master);
      return TRUE;
    }
  }

  // auto stand if down
  if (GET_POS(ch) < POS_FIGHTING && GET_POS(ch) >= POS_STUNNED)
  {
    change_position(ch, POS_STANDING);
    act("$n clambers to $s feet.\r\n", FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  // we're fighting something we dont want to fight...
  if (!ch->master && FIGHTING(ch) && IS_NPC(FIGHTING(ch)) && !IS_PET(FIGHTING(ch)))
    do_flee(ch, 0, 0, 0);

  return FALSE;
}

/* from Homeland */
SPECIAL(wraith_elemental)
{
  struct char_data *vict;

  if (cmd)
    return FALSE;

  if (GET_POS(ch) == POS_DEAD || (!ch->master && !MOB_FLAGGED(ch, MOB_MEMORY)))
  {
    act("With a loud shriek, $n returns to $s home plane.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (GET_HIT(ch) > 0)
  {
    if (ch->master && ch->in_room == ch->master->in_room && !rand_number(0, 1))
    {
      for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
      {
        if (FIGHTING(vict) == ch->master)
        {
          perform_rescue(ch, ch->master);
          return TRUE;
        }
      }
    }

    if (!FIGHTING(ch) && ch->master && FIGHTING(ch->master) && ch->in_room == ch->master->in_room)
    {
      perform_assist(ch, ch->master);
      return TRUE;
    }
  }

  // auto stand if down
  if (GET_POS(ch) < POS_FIGHTING && GET_POS(ch) >= POS_STUNNED)
  {
    change_position(ch, POS_STANDING);
    act("$n clambers to $s feet.\r\n", FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  // we're fighting something we dont want to fight...
  if (!ch->master && FIGHTING(ch) && IS_NPC(FIGHTING(ch)) && !IS_PET(FIGHTING(ch)))
    do_flee(ch, 0, 0, 0);

  return FALSE;
}

/* from homeland */
SPECIAL(planewalker)
{
  if (cmd)
    return FALSE;

  if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
  {
    act("$n looks around in panic when he realizes that his spells\r\n"
        "would fizzle. He reaches down into his pockets and pulls out an ancient\r\n"
        "rod. He taps the rod and suddenly disappears!",
        FALSE, ch, 0, 0, TO_ROOM);
    call_magic(ch, 0, 0, SPELL_TELEPORT, 0, 30, CAST_WAND);
    return TRUE;
  }
  if (!FIGHTING(ch) && GET_HIT(ch) < GET_MAX_HIT(ch))
  {
    act("$n checks on his wounds, and grabs a potion from his pockets.", FALSE, ch, 0, 0, TO_ROOM);
    call_magic(ch, ch, 0, SPELL_HEAL, 0, 30, CAST_POTION);
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(phantom)
{
  struct char_data *vict;
  struct char_data *next_vict;
  int prob, percent;

  if (cmd)
    return FALSE;

  if (!FIGHTING(ch))
    return FALSE;
  if (rand_number(0, 4))
    return FALSE;

  act("$n \tLlets out a \trfrightening\tL wail\tn", FALSE, ch, 0, 0, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;

    if (vict == ch)
      continue;
    if (IS_NPC(vict) && !IS_PET(vict))
      continue;

    percent = rand_number(1, 111); /* 101% is a complete failure */
    prob = GET_WIS(vict) + 5;
    if (FIGHTING(vict))
      prob *= 2;
    if (prob > 100)
      prob = 100;

    if (percent > prob)
      do_flee(vict, NULL, 0, 0);
  }
  return TRUE;
}

/* this is the old lichdrain, don't think it works in its current
   implementation */
int perform_lichdrain(struct char_data *ch)
{
  if (!ch)
    return 0;

  struct char_data *tch = 0;
  struct char_data *vict = 0;
  int dam = 0;

  if (GET_POS(ch) == POS_DEAD)
    return FALSE;
  if (rand_number(0, 3))
    return FALSE;
  if (!FIGHTING(ch))
    return FALSE;

  if (AFF_FLAGGED(ch, AFF_PARALYZED))
    return FALSE;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (!IS_NPC(tch) || IS_PET(tch))
    {
      if (!vict || !rand_number(0, 2))
      {
        vict = tch;
      }
    }
  }

  if (!vict)
    return FALSE;

  act("\tn$n\tL looks deep into your soul with $s horrid gaze.\tn\r\n"
      "\tLand $e simply leeches your \tWlifeforce\tL out of you.\r\n",
      FALSE, ch, 0, vict, TO_VICT);

  act("\tn$n\tL looks deep into the eyes of $N\tL with $s horrid gaze.\tn\r\n"
      "\tLand $e simply leeches $S \tWlifeforce\tL out of $M.\r\n",
      TRUE, ch, 0, vict, TO_NOTVICT);

  act("\tWYou reach out and suck the life force away from $N!", TRUE, ch, 0, vict, TO_CHAR);
  dam = GET_HIT(vict) + 5;
  if (GET_HIT(ch) + dam < GET_MAX_HIT(ch))
    GET_HIT(ch) += dam;
  GET_HIT(vict) -= dam;
  USE_FULL_ROUND_ACTION(vict);
  return TRUE;
}

/* threw this together so the experience of encountering lich isn't a pleasant one :P */
SPECIAL(lich_mob)
{
  struct char_data *vict = NULL;
  int use_aoe = 0;

  if (!ch)
    return 0;

  /* note that the !vict is moved below */
  if (cmd)
    return 0;

  /* find random target, and num targets */
  if (!(vict = npc_find_target(ch, &use_aoe)))
    return 0;

  /* this is the offensive arsenal */
  if (vict && rand_number(0, 1))
  {
    if (!rand_number(0, 5))
    {
      act("\tWWith power and determination you unleash an aggressive BURST of magic!\tn", TRUE, ch,
          0, FIGHTING(ch), TO_CHAR);
      act("$n\tL, with power and determination, unleashes an aggressive BURST of magic!\tn", FALSE,
          ch, 0, FIGHTING(ch), TO_VICT);
      act("$n\tL, with power and determination, unleashes an aggressive BURST of magic!\tn", TRUE,
          ch, 0, FIGHTING(ch), TO_NOTVICT);

      /* looks like the swarm worked */
      if (call_magic(ch, vict, 0, SPELL_METEOR_SWARM, 0, GET_LEVEL(ch), CAST_INNATE))
        return 1;
    }
    else if (!rand_number(0, 2) && (!IS_UNDEAD(vict) && !IS_LICH(vict)) &&
             perform_lichtouch(ch, vict))
    {
      /* looks like we did the lichtouch! */
      return 1;
    }
    else if (!rand_number(0, 2) && (IS_UNDEAD(ch) || IS_LICH(ch)) && perform_lichtouch(ch, ch))
    {
      /* looks like we did the self healing lichtouch */
      return 1;
    }
    else if (!rand_number(0, 4))
    {
      int i = 0;

      act("\tWWith power and determination you unleash an aggressive flurry of magic!\tn", TRUE, ch,
          0, FIGHTING(ch), TO_CHAR);
      act("$n\tL, with power and determination, unleashes an aggressive flurry of magic!\tn", FALSE,
          ch, 0, FIGHTING(ch), TO_VICT);
      act("$n\tL, with power and determination, unleashes an aggressive flurry of magic!\tn", TRUE,
          ch, 0, FIGHTING(ch), TO_NOTVICT);

      /* spam some nukes! */
      for (i = 0; i <= rand_number(1, 3); i++)
      {
        if (valid_fight_cond(ch, TRUE))
        {
          switch (rand_number(0, 2))
          {
          case 0:
            call_magic(ch, vict, 0, SPELL_PRISMATIC_SPRAY, 0, GET_LEVEL(ch), CAST_INNATE);
            break;
          case 1:
            call_magic(ch, vict, 0, SPELL_CHAIN_LIGHTNING, 0, GET_LEVEL(ch), CAST_INNATE);
            break;
          default:
            call_magic(ch, vict, 0, SPELL_THUNDERCLAP, 0, GET_LEVEL(ch), CAST_INNATE);
            break;
          }
        }
      }
      return 1;
    }
  }

  return 0;
}

/* from homeland */
SPECIAL(bonedancer)
{
  struct char_data *vict;
  struct char_data *next_vict;

  if (cmd)
    return FALSE;
  if (GET_POS(ch) == POS_DEAD || !ch->master)
  {
    act("With a loud shriek, $n crumbles into dust.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (!FIGHTING(ch) && GET_HIT(ch) > 0)
  {
    for (vict = world[ch->in_room].people; vict; vict = next_vict)
    {
      next_vict = vict->next_in_room;
      if (vict != ch && CAN_SEE(ch, vict))
      {
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        return TRUE;
      }
    }
  }

  return FALSE;
}

/* from homeland */
SPECIAL(mercenary)
{
  int hit;
  int base = 1;

  if (!ch)
    return FALSE;
  if (cmd)
    return FALSE;

  // a recruited merc should get reasonable amounts of hp.
  if (PROC_FIRED(ch) == FALSE && IS_PET(ch))
  {
    switch (GET_CLASS(ch))
    {
    case CLASS_RANGER:
    case CLASS_PALADIN:
    case CLASS_BERSERKER:
    case CLASS_WARRIOR:
    case CLASS_WEAPON_MASTER:
    case CLASS_STALWART_DEFENDER:
    case CLASS_DUELIST:
      base = 8;
      break;
    case CLASS_ROGUE:
      //      case CLASS_SHADOW_DANCER:
      //      case CLASS_ASSASSIN:
    case CLASS_MONK:
    case CLASS_SACRED_FIST:
    case CLASS_SHIFTER:
      base = 5;
      break;

    default:
      base = 3;
      break;
    }

    hit = dice(GET_LEVEL(ch), (1 + GET_CON_BONUS(ch))) + GET_LEVEL(ch) * base;
    GET_MAX_HIT(ch) = hit;
    if (GET_HIT(ch) > hit)
      GET_HIT(ch) = hit;
    PROC_FIRED(ch) = TRUE;
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(ethereal_pet)
{
  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;
  if (FIGHTING(ch))
    return FALSE;

  if (ch->desc == 0)
  {
    extract_char(ch);
    return TRUE;
  }
  return FALSE;
}

SPECIAL(dog)
{
  int random = 0;
  struct affected_type af;
  struct char_data *pet = (struct char_data *)me;

  if (!argument)
    return FALSE;
  if (!cmd)
    return FALSE;

  skip_spaces(&argument);

  if (!isname(argument, GET_NAME(pet)))
    return FALSE;

  if (CMD_IS("pet") || CMD_IS("pat"))
  {
    random = dice(1, 3);
    switch (random)
    {
    case 3:
      act("$n tries to lick your hand as you pet $m.", FALSE, pet, 0, ch, TO_VICT);
      act("$n tries to lick the hand of $N as $E pet $m.", FALSE, pet, 0, ch, TO_NOTVICT);
      break;
    case 2:
      act("$n looks at you with adoring eyes as you pet $m.", FALSE, pet, 0, ch, TO_VICT);
      act("$n looks at $N with adoring eyes as $E pet $m.", FALSE, pet, 0, ch, TO_NOTVICT);
      break;
    case 1:
    default:
      act("$n wags $s tail happily, as you pet $m.", FALSE, pet, 0, ch, TO_VICT);
      act("$n wags $s tail happily, as $N pets $m.", FALSE, pet, 0, ch, TO_NOTVICT);
      break;
    }

    if (GET_LEVEL(pet) < 2 && ch->followers == 0 && ch->master == 0 && pet->master == 0 &&
        !circle_follow(pet, ch))
    {
      add_follower(pet, ch);
      af.spell = SPELL_CHARM;
      af.duration = 24000;
      af.modifier = 0;
      af.location = 0;
      SET_BIT_AR(af.bitvector, AFF_CHARM);
      affect_to_char(pet, &af);
    }
    return TRUE;
  }
  return FALSE;
}
