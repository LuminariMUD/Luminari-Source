/**************************************************************************
 *  File: act.wizard.set.c                             Part of LuminariMUD *
 *  Usage: The staff "set" command: edit a live character's fields.        *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

/* One staff command family, split out of act.wizard.c.
 *
 * The seam is parsing versus execution. do_set() parses the command line and
 * resolves the target; find_set_field() maps a name onto the set_fields[]
 * table; perform_set() performs the mutation. Adding a settable field is a
 * change to set_fields[] plus one case in perform_set(), and it no longer
 * touches any other staff command.
 *
 * Ownership and lifetime
 * ----------------------
 * perform_set() mutates the victim in place and never takes ownership of it:
 * the caller resolved it and the player list owns it. Fields that hold
 * strings are replaced through the same helpers the rest of the codebase
 * uses, so the character keeps owning its own storage. val_arg is borrowed
 * for the duration of the call and is not retained.
 *
 * set_fields[] is static, immutable, and terminated by a "\n" sentinel that
 * find_set_field() relies on. It is indexed by the mode value that
 * find_set_field() returns, so the two must stay in step.
 *
 * Errors, nullability and reentrancy
 * ----------------------------------
 * ch and vict must be non-NULL. find_set_field() returns -1 for an unknown or
 * empty field; perform_set() returns 0 when it rejected the change (and has
 * already told ch why) and non-zero when it applied one. Everything here runs
 * on the game loop thread, mutates live game state, and is not thread-safe.
 */

#include "conf.h"
#include "core/sysdep.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/interpreter.h"
#include "core/handler.h"
#include "core/db.h"
#include "core/constants.h"
#include "core/screen.h"
#include "core/modify.h"
#include "core/mudlim.h"
#include "act.h"

#include "character/class.h"
#include "character/race.h"
#include "character/backgrounds.h"
#include "character/deities.h"
#include "character/feats.h"
#include "character/premadebuilds.h"
#include "character/rewards.h"
#include "clan/clan.h"
#include "dgscript/dg_scripts.h"
#include "magic/spells.h"
#include "magic/spell_prep.h"
#include "magic/domains_schools.h"
#include "obj/house.h"
#include "olc/oasis.h"
#include "player/account.h"
#include "player/ban.h"
#include "player/password.h"
#include "quest/quest.h"
#include "player/player_rename.h"

/* set_fields[] index of the "name" pseudo-field, which perform_set() routes
 * through the rename machinery instead of writing directly. */
#define SET_NAME_FIELD 34

/* The do_set function */

#define PC 1
#define NPC 2
#define BOTH 3

#define MISC 0
#define BINARY 1
#define NUMBER 2
#define ADDER 3

#define SET_CLASS_LEVEL(class_num) ((class_num) + 1)

#define SET_OR_REMOVE(flagset, flags)                                                              \
  {                                                                                                \
    if (on)                                                                                        \
      SET_BIT_AR(flagset, flags);                                                                  \
    else if (off)                                                                                  \
      REMOVE_BIT_AR(flagset, flags);                                                               \
  }

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))

/* The set options available */
static const struct set_struct
{
  const char *cmd;
  const char level;
  const char pcnpc;
  const char type;
  /* Stored one-based so zero means this is not a class-level field. */
  const int class_num_plus_one;
} set_fields[] = {
    {"ac", LVL_BUILDER, BOTH, NUMBER, 0}, /* 0  */
    {"afk", LVL_BUILDER, PC, BINARY, 0},  /* 1  */
    {"age", LVL_STAFF, BOTH, NUMBER, 0},
    {"align", LVL_BUILDER, BOTH, NUMBER, 0},
    {"bank", LVL_BUILDER, PC, NUMBER, 0},
    {"brief", LVL_STAFF, PC, BINARY, 0}, /* 5  */
    {"cha", LVL_IMPL, BOTH, NUMBER, 0},
    {"clan", LVL_STAFF, PC, NUMBER, 0},
    {"clanrank", LVL_STAFF, PC, NUMBER, 0},
    {"class", LVL_IMPL, BOTH, MISC, 0},
    {"color", LVL_STAFF, PC, BINARY, 0},
    {"con", LVL_IMPL, BOTH, NUMBER, 0},
    {"damroll", LVL_IMPL, BOTH, NUMBER, 0}, /* 12 */
    {"deleted", LVL_IMPL, PC, BINARY, 0},
    {"dex", LVL_BUILDER, BOTH, NUMBER, 0},
    {"drunk", LVL_BUILDER, BOTH, MISC, 0},
    {"exp", LVL_IMPL, BOTH, NUMBER, 0},
    {"frozen", LVL_GRSTAFF, PC, BINARY, 0}, /* 17 */
    {"gold", LVL_IMPL, BOTH, NUMBER, 0},
    {"height", LVL_BUILDER, BOTH, NUMBER, 0},
    {"hitpoints", LVL_IMPL, BOTH, NUMBER, 0},
    {"hitroll", LVL_IMPL, BOTH, NUMBER, 0},
    {"hunger", LVL_BUILDER, BOTH, MISC, 0}, /* 22 */
    {"int", LVL_BUILDER, BOTH, NUMBER, 0},
    {"invis", LVL_STAFF, PC, NUMBER, 0},
    {"invstart", LVL_BUILDER, PC, BINARY, 0},
    {"killer", LVL_STAFF, PC, BINARY, 0},
    {"level", LVL_GRSTAFF, BOTH, NUMBER, 0}, /* 27 */
    {"loadroom", LVL_BUILDER, PC, MISC, 0},
    {"psp", LVL_IMPL, BOTH, NUMBER, 0},
    {"maxhit", LVL_IMPL, BOTH, NUMBER, 0},
    {"maxpsp", LVL_IMPL, BOTH, NUMBER, 0},
    {"maxmove", LVL_IMPL, BOTH, NUMBER, 0}, /* 32 */
    {"move", LVL_IMPL, BOTH, NUMBER, 0},
    {"name", LVL_IMMORT, PC, MISC, 0},
    {"nodelete", LVL_STAFF, PC, BINARY, 0},
    {"nohassle", LVL_STAFF, PC, BINARY, 0},
    {"nosummon", LVL_BUILDER, PC, BINARY, 0}, /* 37 */
    {"nowizlist", LVL_GRSTAFF, PC, BINARY, 0},
    {"olc", LVL_GRSTAFF, PC, MISC, 0},
    {"password", LVL_GRSTAFF, PC, MISC, 0},
    {"poofin", LVL_IMMORT, PC, MISC, 0},
    {"poofout", LVL_IMMORT, PC, MISC, 0}, /* 42 */
    {"practices", LVL_IMPL, PC, NUMBER, 0},
    {"quest", LVL_STAFF, PC, NUMBER, 0},
    {"room", LVL_BUILDER, BOTH, NUMBER, 0},
    {"screenwidth", LVL_STAFF, PC, NUMBER, 0},
    {"sex", LVL_STAFF, BOTH, MISC, 0}, /* 47 */
    {"showvnums", LVL_BUILDER, PC, BINARY, 0},
    {"siteok", LVL_STAFF, PC, BINARY, 0},
    {"str", LVL_IMPL, BOTH, NUMBER, 0},
    {"stradd", LVL_IMPL, BOTH, NUMBER, 0},
    {"thief", LVL_STAFF, PC, BINARY, 0}, /* 52 */
    {"thirst", LVL_BUILDER, BOTH, MISC, 0},
    {"title", LVL_STAFF, PC, MISC, 0},
    {"variable", LVL_GRSTAFF, PC, MISC, 0},
    {"weight", LVL_BUILDER, BOTH, NUMBER, 0},
    {"wis", LVL_IMPL, BOTH, NUMBER, 0}, /* 57 */
    {"questpoints", LVL_IMPL, PC, NUMBER, 0},
    {"questhistory", LVL_STAFF, PC, NUMBER, 0},
    {"trains", LVL_IMPL, PC, NUMBER, 0}, /* 60 */
    {"race", LVL_IMPL, BOTH, MISC, 0},
    {"spellres", LVL_IMPL, PC, NUMBER, 0},                                                /* 62 */
    {"size", LVL_IMPL, PC, NUMBER, 0},                                                    /* 63 */
    {"wizard", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_WIZARD)},                      /* 64 */
    {"cleric", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_CLERIC)},                      /* 65 */
    {"rogue", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_ROGUE)},                        /* 66 */
    {"warrior", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_WARRIOR)},                    /* 67 */
    {"monk", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_MONK)},                          /* 68 */
    {"druid", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_DRUID)},                        /* 69 */
    {"boost", LVL_IMPL, PC, NUMBER, 0},                                                   /* 70 */
    {"berserker", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_BERSERKER)},                /* 71 */
    {"sorcerer", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_SORCERER)},                  /* 72 */
    {"paladin", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_PALADIN)},                    /* 73 */
    {"ranger", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_RANGER)},                      /* 74 */
    {"bard", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_BARD)},                          /* 75 */
    {"featpoints", LVL_IMPL, PC, NUMBER, 0},                                              /* 76 */
    {"epicfeatpoints", LVL_IMPL, PC, NUMBER, 0},                                          /* 77 */
    {"classfeats", LVL_IMPL, PC, MISC, 0},                                                /* 78 */
    {"epicclassfeats", LVL_IMPL, PC, MISC, 0},                                            /* 79 */
    {"accexp", LVL_IMPL, PC, NUMBER, 0},                                                  /* 80 */
    {"weaponmaster", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_WEAPON_MASTER)},         /* 81 */
    {"arcanearcher", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_ARCANE_ARCHER)},         /* 82 */
    {"stalwartdefender", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_STALWART_DEFENDER)}, /* 83 */
    {"shifter", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_SHIFTER)},                    /* 84 */
    {"duelist", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_DUELIST)},                    /* 85 */
    {"guimode", LVL_BUILDER, PC, BINARY, 0},                                              /* 86 */
    {"rpmode", LVL_BUILDER, PC, BINARY, 0},                                               /* 87 */
    {"mystictheurge", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_MYSTIC_THEURGE)},       /* 88 */
    {"addaccexp", LVL_IMPL, PC, ADDER, 0},                                                /* 89 */
    {"alchemist", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_ALCHEMIST)},                /* 90 */
    {"arcaneshadow", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_ARCANE_SHADOW)},         /* 91 */
    {"sacredfist", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_SACRED_FIST)},             /* 92 */
    {"premadebuild", LVL_STAFF, PC, MISC, 0},                                             /* 93 */
    {"psionicist", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_PSIONICIST)},              /* 94 */
    {"deity", LVL_BUILDER, PC, MISC, 0},                                                  /* 95 */
    {"eldritchknight", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_ELDRITCH_KNIGHT)},     /* 96 */
    {"spellsword", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_SPELLSWORD)},              /* 97 */
    {"shadowdancer", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_SHADOW_DANCER)},         /* 98 */
    {"blackguard", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_BLACKGUARD)},              /* 99 */
    {"assassin", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_ASSASSIN)},                  /* 100 */
    {"inquisitor", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_INQUISITOR)},              /* 101 */
    {"homeland", LVL_STAFF, PC, NUMBER, 0},                                               /* 102 */
    {"region", LVL_STAFF, PC, NUMBER, 0},                                                 /* 103 */
    {"shortdesc", LVL_STAFF, PC, MISC, 0},                                                /* 104 */
    {"necromancer", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_NECROMANCER)},            /* 105 */
    {"background", LVL_STAFF, PC, MISC, 0},                                               /* 106 */
    {"arcanemark", LVL_STAFF, PC, MISC, 0},                                               /* 107 */
    {"arcaneschool", LVL_STAFF, PC, MISC, 0},                                             /* 108 */
    {"summoner", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_SUMMONER)},                  /* 109 */
    {"warlock", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_WARLOCK)},                    /* 110 */
    {"knightoftheluminousthread", LVL_IMPL, PC, NUMBER,
     SET_CLASS_LEVEL(CLASS_KNIGHT_OF_SOLAMNIA)}, /* 111 */
    {"knightoftheshatteredmirror", LVL_IMPL, PC, NUMBER,
     SET_CLASS_LEVEL(CLASS_KNIGHT_OF_THE_THORN)}, /* 112 */
    {"knightofthepalethrone", LVL_IMPL, PC, NUMBER,
     SET_CLASS_LEVEL(CLASS_KNIGHT_OF_THE_SKULL)}, /* 113 */
    {"knightofthehowlingmoon", LVL_IMPL, PC, NUMBER,
     SET_CLASS_LEVEL(CLASS_KNIGHT_OF_THE_LILY)},                               /* 114 */
    {"dragonrider", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_DRAGONRIDER)}, /* 115 */
    {"artificer", LVL_IMPL, PC, NUMBER, SET_CLASS_LEVEL(CLASS_ARTIFICER)},     /* 116 */

    {"\n", 0, BOTH, MISC, 0},
};

static int find_set_field(const char *field)
{
  size_t len;
  int mode;

  if (field == NULL || *field == '\0')
    return (-1);

  len = strlen(field);
  for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
  {
    if (!strncmp(field, set_fields[mode].cmd, len))
      return (mode);
  }

  return (-1);
}

static int perform_set(struct char_data *ch, struct char_data *vict, int mode, char *val_arg)
{
  int i, on = 0, off = 0, value = 0, qvnum;
  room_rnum rnum;
  room_vnum rvnum;
  qst_rnum qnum;
  char arg1[MAX_INPUT_LENGTH] = {'\0'}, arg2[MAX_INPUT_LENGTH] = {'\0'};
  int class = CLASS_UNDEFINED;
  char buf1[LONG_STRING];

  /* Check to make sure all the levels are correct */
  if (GET_LEVEL(ch) != LVL_IMPL)
  {
    if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && vict != ch)
    {
      send_to_char(ch, "Maybe that's not such a great idea...\r\n");
      return (0);
    }
  }
  if (GET_LEVEL(ch) < set_fields[mode].level)
  {
    send_to_char(ch, "You are not godly enough for that!\r\n");
    return (0);
  }

  /* Make sure the PC/NPC is correct */
  if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC))
  {
    send_to_char(ch, "You can't do that to a beast!\r\n");
    return (0);
  }
  else if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC))
  {
    send_to_char(ch, "That can only be done to a beast!\r\n");
    return (0);
  }

  /* Find the value of the argument */
  if (set_fields[mode].type == BINARY)
  {
    if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
      on = 1;
    else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
      off = 1;
    if (!(on || off))
    {
      send_to_char(ch, "Value must be 'on' or 'off'.\r\n");
      return (0);
    }
  }
  else if (set_fields[mode].type == NUMBER || set_fields[mode].type == ADDER)
  {
    value = atoi(val_arg);
  }

  if (set_fields[mode].class_num_plus_one != 0)
  {
    class = set_fields[mode].class_num_plus_one - 1;
    if (set_fields[mode].type != NUMBER || class < 0 || class >= NUM_CLASSES)
    {
      log("SYSERR: Invalid class-level set field mapping for '%s'.", set_fields[mode].cmd);
      send_to_char(ch, "Can't set that!\r\n");
      return (0);
    }

    CLASS_LEVEL(vict, class) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    send_to_char(ch, "%s's %s set to %d.\r\n", GET_NAME(vict), set_fields[mode].cmd, value);
    return (1);
  }

  switch (mode)
  {
  case 0:                              /* ac */
    GET_REAL_AC(vict) = RANGE(0, 300); /* 0-30AC */
    affect_total(vict);
    break;
  case 1: /* afk */
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_AFK);
    break;
  case 2: /* age */
    if (value < 2 || value > 200)
    { /* Arbitrary limits. */
      send_to_char(ch, "Ages 2 to 200 accepted.\r\n");
      return (0);
    }
    /* NOTE: May not display the exact age specified due to the integer
     * division used elsewhere in the code.  Seems to only happen for
     * some values below the starting age (17) anyway. -gg 5/27/98 */
    vict->player.time.birth = time(0) - ((value - 17) * SECS_PER_MUD_YEAR);
    break;
  case 3: /* align */
    GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
    affect_total(vict);
    break;
  case 4: /* bank */
    award_set_points(vict, AWARD_BANK_GOLD, RANGE(0, 100000000));
    break;
  case 5: /* brief */
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
    break;
  case 6: /* cha */
    RANGE(3, 50);
    GET_REAL_CHA(vict) = value;
    affect_total(vict);
    break;
  case 7: /* clan */
    if (value == 0 || !str_cmp(val_arg, "off"))
    {
      GET_CLAN(vict) = 0;
      GET_CLANRANK(vict) = 0;

      /* Set value in pindex */
      for (i = 0; i < top_of_p_table; i++)
        if (player_table[i].id == GET_IDNUM(vict))
          player_table[i].clan = 0;
      save_player_index();
      send_to_char(ch, "%s is now in no clan.\r\n", GET_NAME(vict));
      break;
    }
    else if ((i = real_clan(value)) == (int)NO_CLAN)
    {
      send_to_char(ch, "Invalid clan VNUM!\r\nSee clan info for a list.\r\n");
      return (0);
    }
    set_clan(vict, value);
    send_to_char(ch, "%s is now in the '%s%s' clan.\r\n", GET_NAME(vict), clan_list[i].clan_name,
                 CCNRM(ch, C_NRM));
    break;
  case 8: /* clanrank */
    if ((i = real_clan(GET_CLAN(vict))) == (int)NO_CLAN)
    {
      send_to_char(ch, "%s isn't in a clan, so can't have a rank!\r\n", GET_NAME(vict));
      return (0);
    }
    GET_CLANRANK(vict) = RANGE(0, clan_list[i].ranks);
    break;
  case 9: /* class */
    if ((i = parse_class_long(val_arg)) == CLASS_UNDEFINED)
    {
      send_to_char(ch, "That is not a class.\r\n");
      return (0);
    }
    GET_CLASS(vict) = i;
    break;
  case 10: /* color */
    SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_1));
    SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_2));
    break;
  case 11: /* con */
    RANGE(3, 50);
    GET_REAL_CON(vict) = value;
    affect_total(vict);
    break;
  case 12: /* damroll */
    GET_REAL_DAMROLL(vict) = RANGE(-20, 20);
    affect_total(vict);
    break;
  case 13: /* delete */
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
    break;
  case 14: /* dex */
    RANGE(3, 50);
    GET_REAL_DEX(vict) = value;
    affect_total(vict);
    break;
  case 15: /* drunk */
    if (!str_cmp(val_arg, "off"))
    {
      GET_COND(vict, DRUNK) = -1;
      send_to_char(ch, "%s's drunkenness is now off.\r\n", GET_NAME(vict));
    }
    else if (is_number(val_arg))
    {
      value = atoi(val_arg);
      RANGE(0, 24);
      GET_COND(vict, DRUNK) = (sbyte)value;
      send_to_char(ch, "%s's drunkenness set to %d.\r\n", GET_NAME(vict), value);
    }
    else
    {
      send_to_char(ch, "Must be 'off' or a value from 0 to 24.\r\n");
      return (0);
    }
    break;
  case 16: /* exp */
    award_set_points(vict, AWARD_EXPERIENCE, value);
    break;
  case 17: /* frozen */
    if (ch == vict && on)
    {
      send_to_char(ch, "Better not -- could be a long winter!\r\n");
      return (0);
    }
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
    break;
  case 18: /* gold */
    award_set_points(vict, AWARD_GOLD, RANGE(0, 100000000));
    break;
  case 19: /* height */
    GET_HEIGHT(vict) = value;
    affect_total(vict);
    break;
  case 20: /* hit */
    GET_HIT(vict) = RANGE(-9, GET_MAX_HIT(vict));
    affect_total(vict);
    break;
  case 21: /* hitroll */
    GET_REAL_HITROLL(vict) = RANGE(-20, 20);
    affect_total(vict);
    break;
  case 22: /* hunger */
    if (!str_cmp(val_arg, "off"))
    {
      GET_COND(vict, HUNGER) = -1;
      send_to_char(ch, "%s's hunger is now off.\r\n", GET_NAME(vict));
    }
    else if (is_number(val_arg))
    {
      value = atoi(val_arg);
      RANGE(0, 24);
      GET_COND(vict, HUNGER) = (sbyte)value;
      send_to_char(ch, "%s's hunger set to %d.\r\n", GET_NAME(vict), value);
    }
    else
    {
      send_to_char(ch, "Must be 'off' or a value from 0 to 24.\r\n");
      return (0);
    }
    break;
  case 23: /* int */
    RANGE(3, 50);
    GET_REAL_INT(vict) = value;
    affect_total(vict);
    break;
  case 24: /* invis */
    if (GET_LEVEL(ch) < LVL_IMPL && ch != vict)
    {
      send_to_char(ch, "You aren't godly enough for that!\r\n");
      return (0);
    }
    GET_INVIS_LEV(vict) = (sh_int)RANGE(0, GET_LEVEL(vict));
    break;
  case 25: /* invistart */
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
    break;
  case 26: /* killer */
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
    break;
  case 27: /* level */
    if ((!IS_NPC(vict) && value > GET_LEVEL(ch)) || value > LVL_IMPL)
    {
      send_to_char(ch, "You can't do that.\r\n");
      return (0);
    }
    RANGE(1, LVL_IMPL);
    GET_LEVEL(vict) = value;
    break;
  case 28: /* loadroom */
    if (!str_cmp(val_arg, "off"))
    {
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_LOADROOM);
    }
    else if (is_number(val_arg))
    {
      rvnum = atoi(val_arg);
      if (real_room(rvnum) != NOWHERE)
      {
        SET_BIT_AR(PLR_FLAGS(vict), PLR_LOADROOM);
        GET_LOADROOM(vict) = rvnum;
        send_to_char(ch, "%s will enter at room #%" PRI_IDX ".\r\n", GET_NAME(vict),
                     GET_LOADROOM(vict));
      }
      else
      {
        send_to_char(ch, "That room does not exist!\r\n");
        return (0);
      }
    }
    else
    {
      send_to_char(ch, "Must be 'off' or a room's virtual number.\r\n");
      return (0);
    }
    break;
  case 29: /* psp */
    GET_PSP(vict) = RANGE(0, GET_MAX_PSP(vict));
    affect_total(vict);
    break;
  case 30: /* maxhit */
    GET_REAL_MAX_HIT(vict) = RANGE(1, GET_LEVEL(vict) * 500);
    affect_total(vict);
    break;
  case 31: /* maxpsp */
    GET_REAL_MAX_PSP(vict) = RANGE(1, GET_LEVEL(vict) * 500);
    affect_total(vict);
    break;
  case 32: /* maxmove */
    GET_REAL_MAX_MOVE(vict) = RANGE(1, GET_LEVEL(vict) * 500);
    affect_total(vict);
    break;
  case 33: /* move */
    GET_MOVE(vict) = RANGE(0, GET_MAX_MOVE(vict));
    affect_total(vict);
    break;
  case SET_NAME_FIELD: /* name */
    if (ch != vict && GET_LEVEL(ch) < LVL_IMPL)
    {
      send_to_char(ch, "Only Imps can change the name of other players.\r\n");
      return (0);
    }
    if (!change_player_name(ch, vict, val_arg))
      return (0);
    break;
  case 35: /* nodelete */
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
    break;
  case 36: /* nohassle */
    if (GET_LEVEL(ch) < LVL_STAFF && ch != vict)
    {
      send_to_char(ch, "You aren't godly enough for that!\r\n");
      return (0);
    }
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
    break;
  case 37: /* nosummon */
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
    send_to_char(ch, "Nosummon %s for %s.\r\n", ONOFF(!on), GET_NAME(vict));
    break;
  case 38: /* nowiz */
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
    break;
  case 39: /* olc */
    if (is_abbrev(val_arg, "socials") || is_abbrev(val_arg, "actions") ||
        is_abbrev(val_arg, "aedit"))
      GET_OLC_ZONE(vict) = AEDIT_PERMISSION;
    else if (is_abbrev(val_arg, "hedit") || is_abbrev(val_arg, "help"))
      GET_OLC_ZONE(vict) = HEDIT_PERMISSION;
    else if (*val_arg == '*' || is_abbrev(val_arg, "all"))
      GET_OLC_ZONE(vict) = ALL_PERMISSION;
    else if (is_abbrev(val_arg, "off"))
      GET_OLC_ZONE(vict) = NOWHERE;
    else if (!is_number(val_arg))
    {
      send_to_char(ch, "Value must be a zone number, 'aedit', 'hedit', 'off' or 'all'.\r\n");
      return (0);
    }
    else
      GET_OLC_ZONE(vict) = atoi(val_arg);
    break;
  case 40: /* password */
    if (GET_LEVEL(vict) >= LVL_GRSTAFF)
    {
      send_to_char(ch, "You cannot change that.\r\n");
      return (0);
    }
    if (strlen(val_arg) < MIN_PWD_LENGTH || strlen(val_arg) > MAX_PWD_LENGTH)
    {
      send_to_char(ch, "Passwords must be between %d and %d characters.\r\n", MIN_PWD_LENGTH,
                   MAX_PWD_LENGTH);
      return (0);
    }
    if (!password_hash(val_arg, GET_PASSWD(vict), sizeof(vict->player.passwd)))
    {
      send_to_char(ch, "Unable to store that password.\r\n");
      return (0);
    }
    send_to_char(ch, "Password changed.\r\n");
    break;
  case 41: /* poofin */
    if ((vict == ch) || (GET_LEVEL(ch) == LVL_IMPL))
    {
      skip_spaces(&val_arg);
      parse_at(val_arg);

      if (POOFIN(vict))
        free(POOFIN(vict));

      if (!*val_arg)
        POOFIN(vict) = NULL;
      else
        POOFIN(vict) = strdup(val_arg);
    }
    break;
  case 42: /* poofout */
    if ((vict == ch) || (GET_LEVEL(ch) == LVL_IMPL))
    {
      skip_spaces(&val_arg);
      parse_at(val_arg);

      if (POOFOUT(vict))
        free(POOFOUT(vict));

      if (!*val_arg)
        POOFOUT(vict) = NULL;
      else
        POOFOUT(vict) = strdup(val_arg);
    }
    break;
  case 43: /* practices */
    GET_PRACTICES(vict) = RANGE(0, 100);
    break;
  case 44: /* quest */
    value = atoi(val_arg);
    if (IS_NPC(vict))
    {
      send_to_char(ch, "%s is an NPC and this command cannot be used on NPCs.\r\n", GET_NAME(vict));
      return (0);
    }
    if (value <= 0)
    {
      send_to_char(
          ch, "That quest vnum is out of bounds. Must be greater than 1 and less than 65555.\r\n");
      return (0);
    }

    for (i = 0; i < GET_NUM_QUESTS(vict); i++)
    {
      if ((qnum = real_quest(vict->player_specials->saved.completed_quests[i])) != NOTHING)
      {
        if (QST_NUM(qnum) == (qst_vnum)value)
        {
          send_to_char(ch, "Quest %d (%s) has been removed from %s's completed quest history.\r\n",
                       value, QST_NAME(qnum), GET_NAME(vict));
          remove_completed_quest(vict, value);
          set_dialogue_quest_succeeded(vict, value);
          return (1);
        }
      }
    }

    if (((qnum = real_quest(GET_QUEST(vict, 0))) != NOTHING) && QST_NUM(qnum) == (qst_vnum)value)
    {
      send_to_char(ch, "Quest #%d (%s) has been removed from %s's active quests.\r\n", value,
                   QST_NAME(qnum), GET_NAME(vict));
      GET_QUEST(vict, 0) = NOTHING;
    }
    else if (((qnum = real_quest(GET_QUEST(vict, 1))) != NOTHING) &&
             QST_NUM(qnum) == (qst_vnum)value)
    {
      send_to_char(ch, "Quest #%d (%s) has been removed from %s's active quests.\r\n", value,
                   QST_NAME(qnum), GET_NAME(vict));
      GET_QUEST(vict, 1) = NOTHING;
    }
    else if (((qnum = real_quest(GET_QUEST(vict, 2))) != NOTHING) &&
             QST_NUM(qnum) == (qst_vnum)value)
    {
      send_to_char(ch, "Quest #%d (%s) has been removed from %s's active quests.\r\n", value,
                   QST_NAME(qnum), GET_NAME(vict));
      GET_QUEST(vict, 2) = NOTHING;
    }
    else if ((qnum = real_quest(value)) != NOTHING)
    {
      send_to_char(ch, "Quest #%d (%s) has been added to %s's completed quest history.\r\n", value,
                   QST_NAME(qnum), GET_NAME(vict));
      add_completed_quest(vict, value);
    }
    else
    {
      send_to_char(ch, "That quest vnum doesn't exist.\r\n");
      return (0);
    }
    break;
  case 45: /* room */
    if ((rnum = real_room(value)) == NOWHERE)
    {
      send_to_char(ch, "No room exists with that number.\r\n");
      return (0);
    }
    if (IN_ROOM(vict) != NOWHERE)
      char_from_room(vict);

    if (ZONE_FLAGGED(GET_ROOM_ZONE(rnum), ZONE_WILDERNESS))
    {
      X_LOC(vict) = world[rnum].coords[0];
      Y_LOC(vict) = world[rnum].coords[1];
    }

    char_to_room_cause(vict, rnum, ch, DOMAIN_RELOCATION_STAFF, -1);
    break;
  case 46: /* screenwidth */
    GET_SCREEN_WIDTH(vict) = (ubyte)RANGE(40, 200);
    break;
  case 47: /* sex */
    if ((i = search_block(val_arg, genders, FALSE)) < 0)
    {
      send_to_char(ch, "Must be 'male', 'female', or 'neutral'.\r\n");
      return (0);
    }
    GET_SEX(vict) = i;
    break;
  case 48: /* showvnums */
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SHOWVNUMS);
    break;
  case 49: /* siteok */
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
    break;
  case 50: /* str */
    RANGE(3, 50);
    GET_REAL_STR(vict) = value;
    affect_total(vict);
    break;
  case 51: /* stradd */
    send_to_char(ch, "Stradd has been taken out.\r\n");
    affect_total(vict);
    break;
  case 52: /* thief */
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
    break;
  case 53: /* thirst */
    if (!str_cmp(val_arg, "off"))
    {
      GET_COND(vict, THIRST) = -1;
      send_to_char(ch, "%s's thirst is now off.\r\n", GET_NAME(vict));
    }
    else if (is_number(val_arg))
    {
      value = atoi(val_arg);
      RANGE(0, 24);
      GET_COND(vict, THIRST) = (sbyte)value;
      send_to_char(ch, "%s's thirst set to %d.\r\n", GET_NAME(vict), value);
    }
    else
    {
      send_to_char(ch, "Must be 'off' or a value from 0 to 24.\r\n");
      return (0);
    }
    break;
  case 54: /* title */
    set_title(vict, val_arg);
    send_to_char(ch, "%s's title is now: %s\r\n", GET_NAME(vict), GET_TITLE(vict));
    break;
  case 55: /* variable */
    return perform_set_dg_var(ch, vict, val_arg);
    break;
  case 56: /* weight */
    GET_WEIGHT(vict) = value;
    affect_total(vict);
    break;
  case 57: /* wis */
    RANGE(3, 50);
    GET_REAL_WIS(vict) = value;
    affect_total(vict);
    break;
  case 58: /* questpoints */
    award_set_points(vict, AWARD_QUEST_POINTS, RANGE(0, 100000000));
    break;
  case 59: /* questhistory */
    qvnum = atoi(val_arg);
    if (real_quest(qvnum) == NOTHING)
    {
      send_to_char(ch, "That quest doesn't exist.\r\n");
      return FALSE;
    }
    else
    {
      if (is_complete(vict, qvnum))
      {
        remove_completed_quest(vict, qvnum);
        send_to_char(ch, "Quest %d removed from history for player %s.\r\n", qvnum, GET_NAME(vict));
      }
      else
      {
        add_completed_quest(vict, qvnum);
        send_to_char(ch, "Quest %d added to history for player %s.\r\n", qvnum, GET_NAME(vict));
      }
      break;
    }
    break;
  case 60: /* training sessions */
    GET_TRAINS(vict) = RANGE(0, 250);
    break;
  case 61: /* race */
    if ((i = parse_race_long(val_arg)) == RACE_UNDEFINED)
    {
      send_to_char(ch, "That is not a race.\r\n");
      return (0);
    }
    GET_REAL_RACE(vict) = i;
    break;
  case 62: /* spellres spell resistance */
    GET_REAL_SPELL_RES(vict) = RANGE(0, 99);
    affect_total(vict);
    break;
  case 63: // size
    GET_REAL_SIZE(vict) = RANGE(0, NUM_SIZES - 1);
    affect_total(vict);
    break;
  case 70: /* boosts */
    GET_BOOSTS(vict) = (ubyte)RANGE(0, 20);
    break;
  case 76: /* featpoints */
    GET_FEAT_POINTS(vict) = (byte)RANGE(0, 20);
    break;
  case 77: /* epicfeatpoints */
    GET_EPIC_FEAT_POINTS(vict) = (byte)RANGE(0, 20);
    break;
  case 78: /* classfeats (points) */
    two_arguments(val_arg, arg1, sizeof(arg1), arg2,
                  sizeof(arg2)); /* set <name> classfeats <class> <#> */
    class = parse_class_long(arg1);
    if (class == CLASS_UNDEFINED)
    {
      send_to_char(ch, "Invalid class! <example: set zusuk classfeat warrior 2>\r\n");
      return 0;
    }
    value = atoi(arg2);
    GET_CLASS_FEATS(vict, class) = (byte)RANGE(0, 20);
    send_to_char(ch, "%s's %s for %s set to %d.\r\n", GET_NAME(vict), set_fields[mode].cmd, arg1,
                 value);
    break;
  case 79: /* epicclassfeats (points) */
    two_arguments(val_arg, arg1, sizeof(arg1), arg2,
                  sizeof(arg2)); /* set <name> epicclassfeats <class> <#> */
    class = parse_class_long(arg1);
    if (class == CLASS_UNDEFINED)
    {
      send_to_char(ch, "Invalid class! <example: set zusuk epicclassfeat warrior 2>\r\n");
      return 0;
    }
    value = atoi(arg2);
    GET_EPIC_CLASS_FEATS(vict, class) = (byte)RANGE(0, 20);
    send_to_char(ch, "%s's %s for %s set to %d.\r\n", GET_NAME(vict), set_fields[mode].cmd, arg1,
                 value);
    break;
  case 80: /* accexp - account experience */
    if (!vict->desc || !vict->desc->account)
    {
      send_to_char(ch, "Account experience can only be changed for a connected player.\r\n");
      return (0);
    }
    award_set_points(vict, AWARD_ACCOUNT_EXPERIENCE, RANGE(0, 99999999));
    break;
  case 86: /* GUI Mode */
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_GUI_MODE);
    send_to_char(ch, "GUI Mode %s for %s.\r\n", ONOFF(!on), GET_NAME(vict));
    break;
  case 87: /* PRF_RP */
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_RP);
    break;
  case 89: /* addaccexp - Adds *additional* account experience */
    if (!vict->desc || !vict->desc->account)
    {
      send_to_char(ch, "Account experience can only be changed for a connected player.\r\n");
      return (0);
    }
    award_account_experience(vict, RANGE(0, 9999999));
    break;
  case 93: /* premade build class */
    if ((i = parse_class_long(val_arg)) == CLASS_UNDEFINED)
    {
      send_to_char(ch, "That is not a premade build class.\r\n");
      return (0);
    }
    GET_PREMADE_BUILD_CLASS(vict) = i;
    break;

  case 95:
    for (i = 0; i < NUM_DEITIES; i++)
    {
      if (deity_list[i].pantheon != DEITY_PANTHEON_ALL)
        continue;
      if (is_abbrev(val_arg, deity_list[i].name))
      {
        break;
      }
    }
    if (i < 0 || i >= NUM_DEITIES)
    {
      send_to_char(ch, "There is no deity by that name.\r\n");
      return (0);
    }
    GET_DEITY(vict) = i;
    break;

  case 102:
  case 103:
    if (!*val_arg)
    {
      send_to_char(ch, "\tcRegions of Luminari\tn\r\n\r\n");
      for (i = 1; i < NUM_REGIONS; i++)
      {
        send_to_char(ch, "%-2d) %-20s ", i, regions[i]);
        if (((i - 1) % 3) == 2)
          send_to_char(ch, "\r\n");
      }
      if (((i - 1) % 3) != 2)
        send_to_char(ch, "\r\n");
      send_to_char(ch, "\r\n\r\nRegion selection is mainly a role playign choice, but it also "
                       "awards an associated language and\r\n"
                       "may be integrated into future game systems.\r\n");
      send_to_char(ch, "Type 'quit' to exit out of region selection.\r\n");
      send_to_char(ch,
                   "\r\nRegion Selection (select %d for default if you do not know what to pick): ",
                   REGION_NONE);
    }
    else
    {
      if (value <= 0 || value >= NUM_REGIONS)
      {
        send_to_char(ch, "That is not a valid homeland region.  For a list, type 'homelands'.\r\n");
        return 0;
      }
      GET_REGION(vict) = value;
      snprintf(buf1, sizeof(buf1), "You set $N's homeland to %s.\r\n", regions[value]);
      act(buf1, FALSE, ch, 0, vict, TO_CHAR);
      snprintf(buf1, sizeof(buf1), "$n sets your homeland to %s.\r\n", regions[value]);
      act(buf1, FALSE, ch, 0, vict, TO_VICT);
      save_char(vict, 0);
    }
    break;

  case 104:
    if (!*val_arg)
    {
      send_to_char(ch, "Please specify 'reset' to reset a player's short description.\r\n");
      return (0);
    }
    if (is_abbrev(val_arg, "reset"))
    {
      GET_PC_DESCRIPTOR_1(vict) = 0;
      GET_PC_ADJECTIVE_1(vict) = 0;
      GET_PC_DESCRIPTOR_2(vict) = 0;
      GET_PC_ADJECTIVE_2(vict) = 0;
      act("You reset $N's short description.", FALSE, ch, 0, vict, TO_CHAR);
      act("$n resets your short description. Type 'quit' and select the menu item corresponding to "
          "short description selection.",
          FALSE, ch, 0, vict, TO_VICT);
      save_char(vict, 0);
      break;
    }
    else
    {
      send_to_char(ch, "Please specify 'reset' to reset a player's short description.\r\n");
      return (0);
    }
    break;

  case 106:
    for (i = 0; i < NUM_BACKGROUNDS; i++)
    {
      if (is_abbrev(val_arg, background_list[i].name))
      {
        break;
      }
    }

    if (i >= NUM_BACKGROUNDS)
    {
      send_to_char(ch, "That is not a valid background archtype.\r\n");
      return 0;
    }

    GET_BACKGROUND(vict) = i;
    send_to_char(ch, "You've changed %s's background archytype to %s.\r\n", GET_NAME(vict),
                 background_list[i].name);
    send_to_char(vict, "%s has changed your background archytype to %s.\r\n",
                 CAN_SEE(vict, ch) ? GET_NAME(ch) : "Someone", background_list[i].name);
    break;

  case 107: /* arcanemark */
    if (!*val_arg)
    {
      send_to_char(ch, "Please specify 'reset' to clear a player's arcane mark signature.\r\n");
      return (0);
    }
    if (is_abbrev(val_arg, "reset"))
    {
      if (GET_ARCANE_MARK(vict))
        free(GET_ARCANE_MARK(vict));
      GET_ARCANE_MARK(vict) = NULL;
      send_to_char(ch, "You have cleared %s's arcane mark signature.\r\n", GET_NAME(vict));
      send_to_char(vict, "%s has cleared your arcane mark signature.\r\n",
                   CAN_SEE(vict, ch) ? GET_NAME(ch) : "Someone");
    }
    else
    {
      send_to_char(ch, "Please specify 'reset' to clear a player's arcane mark signature.\r\n");
      return (0);
    }
    break;

  case 108:
    int school;
    for (school = 1; school < NUM_SCHOOLS; school++)
    {
      if (is_abbrev(val_arg, spell_schools_lower[school]))
      {
        break;
      }
    }
    if (school >= NUM_SCHOOLS)
    {
      send_to_char(ch, "That is not a valid spell school.\r\n");
      return 0;
    }
    GET_SPECIALTY_SCHOOL(vict) = (byte)school;
    send_to_char(ch, "You have set %s's spell school to %s.\r\n", GET_NAME(vict),
                 spell_schools[school]);
    send_to_char(vict, "%s has set your spell school to %s.\r\n",
                 CAN_SEE(vict, ch) ? GET_NAME(ch) : "Someone", spell_schools[school]);
    break;

  default:
    send_to_char(ch, "Can't set that!\r\n");
    return (0);
  }
  /* Show the new value of the variable */
  if (set_fields[mode].type == BINARY)
  {
    send_to_char(ch, "%s %s for %s.\r\n", set_fields[mode].cmd, ONOFF(on), GET_NAME(vict));
  }
  else if (set_fields[mode].type == NUMBER)
  {
    send_to_char(ch, "%s's %s set to %d.\r\n", GET_NAME(vict), set_fields[mode].cmd, value);
  }
  else if (set_fields[mode].type == ADDER)
  {
    send_to_char(ch, "%s's %s increased by %d.\r\n", GET_NAME(vict), set_fields[mode].cmd, value);
  }
  else if (mode != SET_NAME_FIELD)
    send_to_char(ch, "%s", CONFIG_OK);

  return (1);
}

#ifdef LUMINARI_CUTEST
int perform_set_class_level_for_test(struct char_data *ch, struct char_data *vict,
                                     const char *field, int level)
{
  char value_arg[32];
  int mode;

  mode = find_set_field(field);
  if (mode < 0 || set_fields[mode].class_num_plus_one == 0)
    return (0);

  snprintf(value_arg, sizeof(value_arg), "%d", level);
  return perform_set(ch, vict, mode, value_arg);
}

int find_set_field_for_test(const char *field)
{
  return find_set_field(field);
}

int set_field_count_for_test(void)
{
  int mode;

  for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
    ;
  return mode;
}

int set_field_min_level_for_test(int mode)
{
  if (mode < 0 || mode >= set_field_count_for_test())
    return (-1);
  return (int)set_fields[mode].level;
}
#endif

static void show_set_help(struct char_data *ch)
{
  const char *const set_levels[] = {"Imm", "God", "GrGod", "IMP"};
  const char *const set_targets[] = {"PC", "NPC", "BOTH"};
  const char *const set_types[] = {"MISC", "BINARY", "NUMBER", "ADDER"};
  char buf[MAX_STRING_LENGTH] = {'\0'};
  int i, len = 0;

  len = snprintf(buf, sizeof(buf), "%sCommand             Lvl    Who?  Type%s\r\n",
                 CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
  for (i = 0; *(set_fields[i].cmd) != '\n'; i++)
  {
    if (set_fields[i].level <= GET_LEVEL(ch))
    {
      len = snprintf_append(buf, sizeof(buf), len, "%-20s%-5s  %-4s  %-6s\r\n", set_fields[i].cmd,
                            set_levels[((int)(set_fields[i].level) - LVL_IMMORT)],
                            set_targets[(int)(set_fields[i].pcnpc) - 1],
                            set_types[(int)(set_fields[i].type)]);
    }
  }
  page_string(ch->desc, buf, TRUE);
}

ACMD(do_set)
{
  struct char_data *vict = NULL, *cbuf = NULL;
  char field[MAX_INPUT_LENGTH] = {'\0'}, name[MAX_INPUT_LENGTH] = {'\0'},
       buf[MAX_INPUT_LENGTH] = {'\0'};
  int mode, player_i = 0, retval;
  char is_file = 0, is_player = 0;

  half_chop_c(argument, name, sizeof(name), buf, sizeof(buf));

  if (!strcmp(name, "file"))
  {
    is_file = 1;
    half_chop(buf, name, buf);
  }
  else if (!str_cmp(name, "help"))
  {
    show_set_help(ch);
    return;
  }
  else if (!str_cmp(name, "player"))
  {
    is_player = 1;
    half_chop(buf, name, buf);
  }
  else if (!str_cmp(name, "mob"))
    half_chop(buf, name, buf);

  half_chop(buf, field, buf);

  if (!*name || !*field)
  {
    send_to_char(ch, "Usage: set <victim> <field> <value>\r\n");
    send_to_char(ch, "       %sset help%s will display valid fields\r\n", CCYEL(ch, C_NRM),
                 CCNRM(ch, C_NRM));
    return;
  }

  /* find the target */
  if (!is_file)
  {
    if (is_player)
    {
      if (!(vict = get_player_vis(ch, name, NULL, FIND_CHAR_WORLD)))
      {
        send_to_char(ch, "There is no such player.\r\n");
        return;
      }
    }
    else
    { /* is_mob */
      if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD)))
      {
        send_to_char(ch, "There is no such creature.\r\n");
        return;
      }
    }
  }
  else if (is_file)
  {
    /* try to load the player off disk */
    CREATE(cbuf, struct char_data, 1);
    clear_char(cbuf);
    CREATE(cbuf->player_specials, struct player_special_data, 1);
    new_mobile_data(cbuf);
    /* Allocate mobile event list */
    // cbuf->events = create_list();
    if ((player_i = load_char(name, cbuf)) > -1)
    {
      if (GET_LEVEL(cbuf) > GET_LEVEL(ch))
      {
        free_char(cbuf);
        send_to_char(ch, "Sorry, you can't do that.\r\n");
        return;
      }
      vict = cbuf;
    }
    else
    {
      free_char(cbuf);
      send_to_char(ch, "There is no such player.\r\n");
      return;
    }
  }

  /* find the command in the list */
  mode = find_set_field(field);
  if (mode < 0)
  {
    retval = 0; /* skips saving below */
    send_to_char(ch, "Can't set that!\r\n");
  }
  else
    /* perform the set */
    retval = perform_set(ch, vict, mode, buf);

  /* save the character if a change was made */
  if (retval)
  {
    if (mode != SET_NAME_FIELD && !is_file && !IS_NPC(vict))
      save_char(vict, 0);
    if (mode != SET_NAME_FIELD && is_file)
    {
      GET_PFILEPOS(cbuf) = player_i;
      save_char(cbuf, 0);
      send_to_char(ch, "Saved in file.\r\n");
    }
  }

  /* free the memory if we allocated it earlier */
  if (is_file)
    free_char(cbuf);
}
