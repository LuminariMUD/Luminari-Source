/**************************************************************************
 *  File: spec/spec_zone_prisoner.c                    Part of LuminariMUD *
 *  Usage: The Prisoner encounter procedures.                             *
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
#include "constants.h"
#include "magic/spells.h"
#include "act.h"
#include "spec_procs.h"
#include "spec_zone_prisoner.h"
#include "combat/fight.h"
#include "actions.h"
#include "combat/spec_abilities.h"
#include "obj/treasure.h"
#include "mob/mob_utils.h"
#include "dgscript/dg_scripts.h"
#include "quest/staff_events.h"

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
    process_healing(vict, ch, -1, (dice(5, 5) + GET_DEX_BONUS(ch)), 0, 0);
    return 1;
  }

  if (vict)
  {
    if (!rand_number(0, 20))
    {
      act("\tWA \tBwave \tWof \tDdarkness \tBoozes \tWslowly from your sword, \tbengulfing \tWthe "
          "\tn\r\n"
          "\tWarea in a \tLvoid \tWof \tLblack.\tW  You begin to perceive the \tBfaint outline "
          "\tn\r\n"
          "\tWof a \tBdragon\tW surrouding your \tbrapier. \tWThe \tBimage \tWbegins to fiercely "
          "\tbclaw \tn\r\n"
          "\tWand \tBsavagely \tbbite \tWat \tn$N's \tWbody.\tn",
          FALSE, ch, 0, vict, TO_CHAR);
      act("\twA \tBwave\tW of \tLdarkness \tBoozes \tWslowly from \tb$n's \tWsword, \tbengulfing "
          "\tWthe \tn\r\n"
          "\tWarea in a \tLvoid \tWof \tLblack.\tW  You begin to perceive the \tBfaint outline "
          "\tn\r\n"
          "\tWof a \tBdragon\tW surrouding \tB&s \tbrapier.\tW  The \tBimage \tWbegins to fiercely "
          "\tbclaw \tn\r\n"
          "\tWand \tBsavagely \tbbite \tWat \tn$N's \tWbody.\tn",
          FALSE, ch, 0, vict, TO_ROOM);
      damage(ch, vict, rand_number(120, 200), -1, DAM_ENERGY, FALSE); // type -1 = no dam message
      return 1;
    }

    if (!rand_number(0, 50))
    {
      weapons_spells(
          "\tWSuddenly your \tn$p\tW is enveloped by \tbsheer \tLdarkness, \tWleaving only a pair "
          "of \tn\r\n"
          "\tBblazing eyes \tWgazing directly into the \tBsoul\tW of \tn$N\tW.  A sudden wave of "
          "\tBterror \tbovercomes \tn\r\n"
          "\tn$N\tW, who begins to \tbtremble violently\tW and lose \tBcontrol \tWof $s senses.\tn",

          "\tW$n's \tWsword is enveloped by \tbsheer \tLdarkness,\tW leaving only a pair of "
          "\tBblazing eyes\tW gazing \tn\r\n"
          "\tWdirectly into the \tBsoul\tW of \tn$N\tW.  A sudden wave of \tBterror \tbovercomes "
          "\tn$n\tW, who begins to \tn\r\n"
          "\tbtremble violenty \tWand lose \tBcontrol\tW of $s senses.\tn",

          "\tW$n's sword is enveloped by \tbsheer \tLdarkness,\tW leaving only a pair of "
          "\tBblazing eyes\tW gazing \tn\r\n"
          "\tWdirectly into the \tBsoul\tb of \tn$N\tW.  A sudden wave of \tBterror \tbovercomes "
          "\tn$N\tW, who begins\tn\r\n"
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
    act("\n\r\tLThe remaining four heads turn and gaze at you with a glare of hatred.\tn", FALSE,
        ch, 0, 0, TO_ROOM);
    prisoner_heads = 4;
    return 1;
  }

  /* white head dies */
  if (prisoner_heads == 4)
  {
    act("\tLYour blood \tWfreezes\tL as the \tWwhite \tLhead of the Prisoner screams\n\r"
        "\tLa horrifying wail of pain and drops to the floor, out of the battle!\tn",
        FALSE, ch, 0, 0, TO_ROOM);
    act("\n\r\tLThe remaining three heads turn and gaze at you with a glare of hatred.\tn", FALSE,
        ch, 0, 0, TO_ROOM);
    prisoner_heads = 3;
    return 1;
  }

  /* black head dies */
  if (prisoner_heads == 3)
  {
    act("\tLYour blood \tWfreezes\tL as the black head of the Prisoner screams\n\r"
        "\tLa horrifying wail of pain and drops to the floor, out of the battle!\tn",
        FALSE, ch, 0, 0, TO_ROOM);
    act("\n\r\tLThe remaining two heads turn and gaze at you with a glare of hatred.\tn", FALSE, ch,
        0, 0, TO_ROOM);
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
  act("\tLSuddenly everything fades to black...\tn", FALSE, prisoner, 0, 0, TO_ROOM);

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

  if (!rand_number(0, 7) && GET_HIT(ch) < GET_MAX_HIT(ch) && PROC_FIRED(ch) == FALSE &&
      !FIGHTING(ch))
  {
    rejuv = GET_HIT(ch) + 1500;

    if (rejuv >= GET_MAX_HIT(ch))
      rejuv = GET_MAX_HIT(ch);

    GET_HIT(ch) = rejuv;

    PROC_FIRED(ch) = TRUE;

    act("\trThe blood-red wounds on the Prisoner's body begin to close as she is partially "
        "revived!\tn",
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
    act("\trThe blood-red wounds on the Prisoner's body begin to close as she is partially "
        "revived!\tn",
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
#define DRAGONKIN_HELM 132136   /* 20 */
#define ASPECT_MASK 132138      /* 21 */
#define GRANDIDIERITE 132141    /* 22 */
#define VOYAGER_BOOTS 132144    /* 23 */
#define WINGED_HELM 132145      /* 24 */
#define DEMON_EYES 132147       /* 25 */
#define HOUND_HELM 132152       /* 26 */
#define PSI_CRYSTAL 132153      /* 27 */
#define ARTIST_SHAWL 132154     /* 28 */
#define VERT_HOOP 132157        /* 29 */
#define PORTABLE_HOLE 132158    /* 30 */
#define DRAGON_WHIP 132161      /* 31 */
#define SHOCK_LANCE 132164      /* 32 */
#define CALAMITY_AXE 132165     /* 33 */
#define TITAN_PICK 132166       /* 34 */
#define TOP_UNIQUES 34
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
#define NUM_TREASURE 8
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
  obj_vnum ovnum = NOTHING;
  int loop_counter = 0, num_items = 0, num_treasure = 0;

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
      DRAGONKIN_HELM,   /* resist scale helm 20 */
      ASPECT_MASK,      /* rogue mask #21 */
      GRANDIDIERITE,    /* arcane necklace 22 */
      VOYAGER_BOOTS,    /* sneak move boots 23 */
      WINGED_HELM,      /* high mental stat caster helm (hood) 24 */
      DEMON_EYES,       /* vision buffs eyewear 25 */
      HOUND_HELM,       /* divine helm 26 */
      PSI_CRYSTAL,      /* psi face 27 */
      ARTIST_SHAWL,     /* bard about 28 */
      VERT_HOOP,        /* arcane ring 29 */
      PORTABLE_HOLE,    /* container 30 */
      DRAGON_WHIP,      /* whip - entangle, silence 31 */
      SHOCK_LANCE,      /* paladin lance 32 */
      CALAMITY_AXE,     /* axe - negatuve energy ray, waves of exhaustion 33 */
      TITAN_PICK,       /* heavy pick - quake, slow, power word stun 34 */
  };

  /* we are giving a random weapon oil, lets have a list of options! */
  int wpnOils[TOP_UNIQUES_OIL + 1] = {
      WEAPON_SPECAB_SEEKING, /* 0 */
      WEAPON_SPECAB_ADAPTIVE,   WEAPON_SPECAB_DISRUPTION,  WEAPON_SPECAB_DEFENDING,
      WEAPON_SPECAB_EXHAUSTING, WEAPON_SPECAB_CORROSIVE, /* 5 */
      WEAPON_SPECAB_SPEED,      WEAPON_SPECAB_GHOST_TOUCH, WEAPON_SPECAB_BLINDING,
      WEAPON_SPECAB_SHOCK,      WEAPON_SPECAB_FLAMING, /* 10 */
      WEAPON_SPECAB_THUNDERING, WEAPON_SPECAB_AGILE,       WEAPON_SPECAB_WOUNDING,
      WEAPON_SPECAB_LUCKY,      WEAPON_SPECAB_BEWILDERING, /* 15 */
      WEAPON_SPECAB_KEEN,       WEAPON_SPECAB_VICIOUS,     WEAPON_SPECAB_INVIGORATING,
      WEAPON_SPECAB_VORPAL,     WEAPON_SPECAB_VAMPIRIC, /* 20 */
      WEAPON_SPECAB_BANE,                               /* 21 */
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
#undef STABILITY_BOOTS
#undef DRAGONKIN_HELM
#undef ASPECT_MASK
#undef GRANDIDIERITE
#undef VOYAGER_BOOTS
#undef WINGED_HELM
#undef DEMON_EYES
#undef HOUND_HELM
#undef PSI_CRYSTAL
#undef ARTIST_SHAWL
#undef VERT_HOOP
#undef PORTABLE_HOLE
#undef DRAGON_WHIP
#undef SHOCK_LANCE
#undef CALAMITY_AXE
#undef TITAN_PICK

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
SPECIAL(prisoner_dracolich)
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
    act("\tLAs the life fades from your body, before collapsing you see is the Prisoner's wicked "
        "grin staring into your soul..\tn",
        FALSE, ch, 0, vict, TO_VICT);
    act("$n \tLturns and gazes at \tn$N\tL, who freezes in place.\tn\r\n"
        "$n \tLreaches out with a skeletal hand and touches \tn$N\tL!\tn",
        TRUE, ch, 0, vict, TO_NOTVICT);
    act("\tL$N\tr SCREAMS\tL in agony, doubling over in pain so intense it makes you "
        "cringe!!\tn\r\n"
        "$n\tL literally sucks the life force from $N,\tn\r\n"
        "\tLwho crumples into a ball of unfathomable pain onto the ground...\tn",
        TRUE, ch, 0, vict, TO_NOTVICT);
    act("\tWWith a grin, you whisper, 'die' at $N, who keels over and falls incapacitated!\tn",
        TRUE, ch, 0, vict, TO_CHAR);

    /* added a way to reduce the effectiveness of this attack -zusuk */
    if (AFF_FLAGGED(vict, AFF_DEATH_WARD) && !rand_number(0, 2))
    {
      hitpoints = damage(ch, vict, rand_number(120, 650), -1, DAM_UNHOLY,
                         FALSE); // type -1 = no dam message
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
