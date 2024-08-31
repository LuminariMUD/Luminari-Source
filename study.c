/* ***********************************************************************
 *    File:   study.c                                Part of LuminariMUD  *
 * Purpose:   To provide menus for class/levelup related features         *
 *            Header info in oasis.h                                      *
 *  Author:   Zusuk                                                       *
 ************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "oasis.h"
#include "screen.h"
#include "interpreter.h"
#include "modify.h"
#include "spells.h"
#include "feats.h"
#include "class.h"
#include "handler.h"
#include "constants.h"
#include "assign_wpn_armor.h"
#include "domains_schools.h"
#include "spell_prep.h"
#include "alchemy.h"
#include "race.h"
#include "premadebuilds.h"
#include "psionics.h"
#include "evolutions.h"
#include "constants.h"

/*-------------------------------------------------------------------*/
/*. Function prototypes . */

static void sorc_study_menu(struct descriptor_data *d, int circle);
static void bard_study_menu(struct descriptor_data *d, int circle);
static void summoner_study_menu(struct descriptor_data *d, int circle);
static void favored_enemy_submenu(struct descriptor_data *d, int favored);
static void favored_enemy_menu(struct descriptor_data *d);
static void animal_companion_menu(struct descriptor_data *d);
static void familiar_menu(struct descriptor_data *d);
static void set_stats_menu(struct descriptor_data *d);

static void display_main_menu(struct descriptor_data *d);
static void generic_main_disp_menu(struct descriptor_data *d);
static void main_feat_disp_menu(struct descriptor_data *d);
static void main_boosts_disp_menu(struct descriptor_data *d);

void init_study(struct descriptor_data *d, int class);
void finalize_study(struct descriptor_data *d);
int compute_ability(struct char_data *ch, int abilityNum);
/*-------------------------------------------------------------------*/

extern const char *cross_names[];

#define MENU_OPT(i) ((i) ? grn : "\tD"), ((i) ? nrm : "\tD")

/* feat types */
#define FEAT_TYPE_NORMAL 1
#define FEAT_TYPE_NORMAL_CLASS 2
#define FEAT_TYPE_EPIC 3
#define FEAT_TYPE_EPIC_CLASS 4
#define FEAT_TYPE_NORMAL_TEAMWORK 5

/* list of possible animal companions, use in-game vnums for this
 * These lists should actually be redesigned to be dynamic, settable
 * via an in-game OLC.  The results should be stored in the database
 * and retrieved when needed. */
#define C_BEAR 60
#define C_BOAR 61
#define C_LION 62
#define C_CROCODILE 63
#define C_HYENA 64
#define C_SNOW_LEOPARD 65
#define C_SKULL_SPIDER 66
#define C_FIRE_BEETLE 67
#define C_CAYHOUND 68
#define C_DRACAVES 69

/*--- paladin mount(s) -----*/
#define C_W_WARHORSE 70
#define C_B_DESTRIER 71
#define C_STALLION 72
#define C_A_DESTRIER 73
#define C_G_WARHORSE 74
#define C_P_WARHORSE 75
#define C_C_DESTRIER 76
#define C_WARDOG 77
#define C_WARPONY 78
#define C_GRIFFON 79
/*--- familiars -----*/
#define F_HUNTER 80
#define F_PANTHER 81
#define F_MOUSE 82
#define F_EAGLE 83
#define F_RAVEN 84
#define F_IMP 85
#define F_PIXIE 86
#define F_FAERIE_DRAGON 87
#define F_PSEUDO_DRAGON 88
#define F_HELLHOUND 89

/*--- sorcerer bloodlines ---*/
#define SB_NONE 0
#define SB_DRACONIC 1
#define SB_ARCANE 2
#define NUM_BLOODLINES 2

/* make a list of vnums corresponding in order, first animals  */
int animal_vnums[] = {
    0,
    C_BEAR,         // 60, 1
    C_BOAR,         // 61, 2
    C_LION,         // 62, 3
    C_CROCODILE,    // 63, 4
    C_HYENA,        // 64, 5
    C_SNOW_LEOPARD, // 65, 6
    C_SKULL_SPIDER, // 66, 7
    C_FIRE_BEETLE,  // 67, 8
    C_CAYHOUND,     // 68, 9
    C_DRACAVES,     // 69, 10
    -1              /* end with this */
};
#define NUM_ANIMALS 10
/* now paladin mounts */
int mount_vnums[] = {
    0,
    C_W_WARHORSE, // 70, 1
    C_B_DESTRIER, // 71, 2
    C_STALLION,   // 72, 3
    C_A_DESTRIER, // 73, 4
    C_G_WARHORSE, // 74, 5
    C_P_WARHORSE, // 75, 6
    C_C_DESTRIER, // 76, 7
    C_WARDOG,     // 77, 8
    C_WARPONY,    // 78, 9
    C_GRIFFON,    // 79, 10
    -1            /* end with this */
};
#define NUM_MOUNTS 10
/* now familiars */
int familiar_vnums[] = {
    0,
    F_HUNTER,        // 80, 1
    F_PANTHER,       // 81, 2
    F_MOUSE,         // 82, 3
    F_EAGLE,         // 83, 4
    F_RAVEN,         // 84, 5
    F_IMP,           // 85, 6
    F_PIXIE,         // 86, 7
    F_FAERIE_DRAGON, // 87, 8
    F_PSEUDO_DRAGON, // 88, 9
    F_HELLHOUND,     // 89, 10
    -1               /* end with this */
};
#define NUM_FAMILIARS 10

/* DEBUG:  just checking first 8 animals right now -zusuk */
#define TOP_OF_C 8
/****************/

/* make a list of names in order, first animals */
const char *animal_names[] = {
    "Unknown",
    "1) Black Bear",
    "2) Boar",
    "3) Lion",
    "4) Crocodile",
    "5) Hyena",
    "6) Snow Leopard",
    "7) Skull Spider",
    "8) Fire Beetle",
    "\n" /* end with this */
};
/* ... now mounts */
const char *mount_names[] = {
    "Unknown",
    "1) Heavy White Warhorse",
    "2) Black Destrier",
    "3) Stallion",
    "4) Armored Destrier",
    "5) Golden Warhorse",
    "6) Painted Warhorse",
    "7) Bright Destrier",
    "8) Wardog",
    "9) Warpony",
    "\n" /* end with this */
};
/* ... now familiars */
const char *familiar_names[] = {
    "Unknown",
    "1) Night Hunter",
    "2) Black Panther",
    "3) Tiny Mouse",
    "4) Eagle",
    "5) Raven",
    "6) Imp",
    "7) Pixie",
    "8) Faerie Dragon",
    "\n" /* end with this */
};

/* NOTE: The above static menus should be converted to dynamic menus.
 * All familiars/companions/etc. are not available to all characters.
 * Additionally, there could be some quest-based familiar/companion
 * choices that might have prerequisites that must be fullfilled prior
 * to their selection. */

/*-------------------------------------------------------------------*\
  utility functions
 \*-------------------------------------------------------------------*/

void init_study(struct descriptor_data *d, int class)
{
  struct char_data *ch = d->character;
  int i = 0, j = 0;

  if (d->olc)
  {
    mudlog(BRF, LVL_IMMORT, TRUE,
           "SYSERR: do_study: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  if (LEVELUP(ch))
  {
    mudlog(BRF, LVL_IMMORT, TRUE,
           "SYSERR: do_study: Player already had levelup structure.");
    free(LEVELUP(ch));
  }

  CREATE(LEVELUP(ch), struct level_data, 1);

  STATE(d) = CON_STUDY;

  /* Now copy the player's data to the levelup structure - a scratch area
   * used during the study process. */
  LEVELUP(ch)->class = class;
  LEVELUP(ch)->level = CLASS_LEVEL(ch, class);
  LEVELUP(ch)->feat_points = MAX(0, GET_FEAT_POINTS(ch));
  LEVELUP(ch)->class_feat_points = MAX(0, GET_CLASS_FEATS(ch, class));
  LEVELUP(ch)->epic_feat_points = MAX(0, GET_EPIC_FEAT_POINTS(ch));
  LEVELUP(ch)->epic_class_feat_points = MAX(0, GET_EPIC_CLASS_FEATS(ch, class));
  LEVELUP(ch)->teamwork_feat_points = MAX(0, GET_TEAMWORK_FEAT_POINTS(ch));

  LEVELUP(ch)->practices = GET_PRACTICES(ch);
  LEVELUP(ch)->trains = GET_TRAINS(ch);
  LEVELUP(ch)->num_boosts = GET_BOOSTS(ch);

  LEVELUP(ch)->str = GET_REAL_STR(ch);
  LEVELUP(ch)->dex = GET_REAL_DEX(ch);
  LEVELUP(ch)->con = GET_REAL_CON(ch);
  LEVELUP(ch)->inte = GET_REAL_INT(ch);
  LEVELUP(ch)->wis = GET_REAL_WIS(ch);
  LEVELUP(ch)->cha = GET_REAL_CHA(ch);

  /* The following data elements are used to store the player's choices during the
   * study process - Just initialize these values. */
  LEVELUP(ch)->spell_circle = -1;
  LEVELUP(ch)->favored_slot = -1;
  LEVELUP(ch)->feat_type = -1;

  LEVELUP(ch)->tempFeat = 0;

  for (i = 0; i < 6; i++)
    LEVELUP(ch)->boosts[i] = 0;
  for (i = 0; i < NUM_FEATS; i++)
  {
    LEVELUP(ch)->feats[i] = 0;
    LEVELUP(ch)->feat_weapons[i] = 0;
    LEVELUP(ch)->feat_skills[i] = 0;
  }
  for (i = 0; i < NUM_CFEATS; i++)
    for (j = 0; j < FT_ARRAY_MAX; j++)
      LEVELUP(ch)->combat_feats[i][j] = 0;
  for (i = 0; i < MAX_ABILITIES; i++)
    for (j = 0; j < NUM_SKFEATS; j++)
      LEVELUP(ch)->skill_focus[i][j] = FALSE;
  for (i = 0; i < NUM_SFEATS; i++)
    LEVELUP(ch)->school_feats[i] = 0;

  LEVELUP(ch)->eidolon_base_form = GET_EIDOLON_BASE_FORM(ch);
  for (i = 1; i < NUM_EVOLUTIONS; i++)
  {
    LEVELUP(ch)->eidolon_evolutions[i] = KNOWS_EVOLUTION(ch, i);
    LEVELUP(ch)->summoner_aspects[i] = HAS_REAL_EVOLUTION(ch, i);
  }
  LEVELUP(ch)->necromancer_bonus_levels = NECROMANCER_CAST_TYPE(ch);

  LEVELUP(ch)->dragon_rider_dragon_type = GET_DRAGON_RIDER_DRAGON_TYPE(ch);
  LEVELUP(ch)->dragon_rider_bond_type = GET_DRAGON_BOND_TYPE(ch);
}

void finalize_study(struct descriptor_data *d)
{
  struct char_data *ch = d->character;
  int i = 0, j = 0, subfeat = 0;
  struct damage_reduction_type *dr;

  /* Finalize the chosen data, applying the levelup structure to
   * the character structure. */
  GET_FEAT_POINTS(ch) = LEVELUP(ch)->feat_points;
  GET_CLASS_FEATS(ch, LEVELUP(ch)->class) = LEVELUP(ch)->class_feat_points;
  GET_EPIC_FEAT_POINTS(ch) = LEVELUP(ch)->epic_feat_points;
  GET_EPIC_CLASS_FEATS(ch, LEVELUP(ch)->class) = LEVELUP(ch)->epic_class_feat_points;
  GET_PRACTICES(ch) = LEVELUP(ch)->practices;
  GET_TRAINS(ch) = LEVELUP(ch)->trains;
  GET_BOOSTS(ch) = LEVELUP(ch)->num_boosts;

  GET_REAL_STR(ch) = LEVELUP(ch)->str;
  GET_REAL_DEX(ch) = LEVELUP(ch)->dex;
  GET_REAL_CON(ch) = LEVELUP(ch)->con;
  GET_REAL_INT(ch) = LEVELUP(ch)->inte;
  GET_REAL_WIS(ch) = LEVELUP(ch)->wis;
  GET_REAL_CHA(ch) = LEVELUP(ch)->cha;

  // assign skill choices
  for (i = START_GENERAL_ABILITIES; i < (END_GENERAL_ABILITIES + 1); i++)
  {
    SET_ABILITY(ch, i, GET_ABILITY(ch, i) + LEVELUP(ch)->skills[i]);
  }
  // assign boost choice
  if (LEVELUP(ch)->boosts[0] > 0)
    GET_REAL_STR(ch) += 1;
  else if (LEVELUP(ch)->boosts[1] > 0)
    GET_REAL_DEX(ch) += 1;
  else if (LEVELUP(ch)->boosts[2] > 0)
    GET_REAL_CON(ch) += 1;
  else if (LEVELUP(ch)->boosts[3] > 0)
    GET_REAL_INT(ch) += 1;
  else if (LEVELUP(ch)->boosts[4] > 0)
    GET_REAL_WIS(ch) += 1;
  else if (LEVELUP(ch)->boosts[5] > 0)
    GET_REAL_CHA(ch) += 1;

  for (i = 0; i < NUM_FEATS; i++)
  {
    if (LEVELUP(ch)->feats[i])
    {

      SET_FEAT(ch, i, HAS_REAL_FEAT(ch, i) + LEVELUP(ch)->feats[i]);

      if ((subfeat = feat_to_skfeat(i)) != -1)
      {
        for (j = 0; j < MAX_ABILITIES + 1; j++)
          if (LEVELUP(ch)->skill_focus[j][subfeat])
            GET_SKILL_FEAT(ch, j, subfeat) = TRUE;
      }

      if ((subfeat = feat_to_cfeat(i)) != -1)
      {
        for (j = 0; j < NUM_WEAPON_TYPES; j++)
          if (HAS_LEVELUP_COMBAT_FEAT(ch, subfeat, j))
            SET_COMBAT_FEAT(ch, subfeat, j);
      }

      if ((subfeat = feat_to_sfeat(i)) != -1)
      {
        for (j = 1; j < NUM_SCHOOLS; j++)
          if (HAS_LEVELUP_SCHOOL_FEAT(ch, subfeat, j))
            SET_SCHOOL_FEAT(ch, subfeat, j);
      }

      /* Handle specific feats here: */
      switch (i)
      {
      case FEAT_MUMMY_DUST:
        send_to_char(ch, "\tMYou gained Epic Spell:  Mummy Dust!\tn\r\n");
        SET_SKILL(ch, SPELL_MUMMY_DUST, 99);
        break;
      case FEAT_DRAGON_KNIGHT:
        send_to_char(ch, "\tMYou gained Epic Spell:  Dragon Knight!\tn\r\n");
        SET_SKILL(ch, SPELL_DRAGON_KNIGHT, 99);
        break;
      case FEAT_GREATER_RUIN:
        send_to_char(ch, "\tMYou gained Epic Spell:  Greater Ruin!\tn\r\n");
        SET_SKILL(ch, SPELL_GREATER_RUIN, 99);
        break;
      case FEAT_HELLBALL:
        send_to_char(ch, "\tMYou gained Epic Spell:  Hellball!\tn\r\n");
        SET_SKILL(ch, SPELL_HELLBALL, 99);
        break;
      case FEAT_EPIC_MAGE_ARMOR:
        send_to_char(ch, "\tMYou gained Epic Spell:  Epic Mage Armor!\tn\r\n");
        SET_SKILL(ch, SPELL_EPIC_MAGE_ARMOR, 99);
        break;
      case FEAT_EPIC_WARDING:
        send_to_char(ch, "\tMYou gained Epic Spell:  Epic Warding!\tn\r\n");
        SET_SKILL(ch, SPELL_EPIC_WARDING, 99);
        break;
      case FEAT_GREAT_CHARISMA:
        GET_REAL_CHA(ch) += LEVELUP(ch)->feats[i];
        break;
      case FEAT_GREAT_CONSTITUTION:
        GET_REAL_CON(ch) += LEVELUP(ch)->feats[i];
        if (!(GET_REAL_CON(ch) % 2) && LEVELUP(ch)->feats[i])
        {
          GET_REAL_MAX_HIT(ch) += GET_LEVEL(ch);
        }
        break;
      case FEAT_GREAT_DEXTERITY:
        GET_REAL_DEX(ch) += LEVELUP(ch)->feats[i];
        break;
      case FEAT_GREAT_INTELLIGENCE:
        GET_REAL_INT(ch) += LEVELUP(ch)->feats[i];
        break;
      case FEAT_GREAT_STRENGTH:
        GET_REAL_STR(ch) += LEVELUP(ch)->feats[i];
        break;
      case FEAT_GREAT_WISDOM:
        GET_REAL_WIS(ch) += LEVELUP(ch)->feats[i];
        break;
      case FEAT_TOUGHNESS:
        for (j = 0; j < GET_LEVEL(ch); j++)
          GET_REAL_MAX_HIT(ch) += LEVELUP(ch)->feats[i];
        break;
      case FEAT_EPIC_TOUGHNESS:
        GET_REAL_MAX_HIT(ch) += 30;
        break;
      case FEAT_EPIC_PSI_MIND:
        GET_REAL_MAX_PSP(ch) += 50;
        break;
      case FEAT_DAMAGE_REDUCTION:
        /* Create the DR structure and attach it to the player. */
        for (dr = GET_DR(ch); dr != NULL; dr = dr->next)
        {
          if (dr->feat == FEAT_DAMAGE_REDUCTION)
          {
            struct damage_reduction_type *temp;
            REMOVE_FROM_LIST(dr, GET_DR(ch), next);
          }
        }
        CREATE(dr, struct damage_reduction_type, 1);
        dr->spell = 0;
        dr->feat = FEAT_DAMAGE_REDUCTION;
        dr->amount = HAS_FEAT(ch, FEAT_DAMAGE_REDUCTION) * 3;
        dr->max_damage = -1;
        dr->bypass_cat[0] = DR_BYPASS_CAT_NONE;
        dr->bypass_val[0] = 0;
        dr->bypass_cat[1] = DR_BYPASS_CAT_UNUSED;
        dr->bypass_val[1] = 0; /* Unused. */
        dr->bypass_cat[2] = DR_BYPASS_CAT_UNUSED;
        dr->bypass_val[2] = 0; /* Unused. */
        dr->next = GET_DR(ch);
        GET_DR(ch) = dr;
        break;
      case FEAT_SORCERER_BLOODLINE_DRACONIC:
        SET_FEAT(ch, FEAT_DRACONIC_HERITAGE_CLAWS, 1);
        SET_FEAT(ch, FEAT_DRACONIC_BLOODLINE_ARCANA, 1);
        break;
      case FEAT_SORCERER_BLOODLINE_ARCANE:
        SET_FEAT(ch, FEAT_IMPROVED_FAMILIAR, HAS_REAL_FEAT(ch, FEAT_IMPROVED_FAMILIAR) + 1);
        SET_FEAT(ch, FEAT_ARCANE_BLOODLINE_ARCANA, 1);
        break;
      }
    }
  } /* for loop running through feats */

  // Assign sorcerer bloodline subtype if necessary
  if (LEVELUP(ch)->sorcerer_bloodline_subtype > 0)
    ch->player_specials->saved.sorcerer_bloodline_subtype = LEVELUP(ch)->sorcerer_bloodline_subtype;

  // Assign high elf cantrip if necessary
  if (LEVELUP(ch)->high_elf_cantrip > 0)
    ch->player_specials->saved.high_elf_cantrip = LEVELUP(ch)->high_elf_cantrip;

  // Assign dragonborn ancestry if necessary
  if (LEVELUP(ch)->dragonborn_draconic_ancestry > 0)
    ch->player_specials->saved.dragonborn_draconic_ancestry = LEVELUP(ch)->dragonborn_draconic_ancestry;

  // Assign chosen alchemist discoveries
  for (i = 0; i < NUM_ALC_DISCOVERIES; i++)
  {
    if (LEVELUP(ch)->discoveries[i])
      ch->player_specials->saved.discoveries[i] = TRUE;
  }

  // Assign chosen paladin mercies
  for (i = 0; i < NUM_PALADIN_MERCIES; i++)
  {
    if (LEVELUP(ch)->paladin_mercies[i])
      ch->player_specials->saved.paladin_mercies[i] = TRUE;
  }

  // Assign chosen blackguard cruelties
  for (i = 0; i < NUM_BLACKGUARD_CRUELTIES; i++)
  {
    if (LEVELUP(ch)->blackguard_cruelties[i])
      ch->player_specials->saved.blackguard_cruelties[i] = TRUE;
  }

  // Assign chosen languages
  for (i = 0; i < NUM_LANGUAGES; i++)
  {
    if (LEVELUP(ch)->languages[i] == TRUE)
    {
      ch->player_specials->saved.languages_known[i] = TRUE;
    }
  }

  NECROMANCER_CAST_TYPE(ch) = LEVELUP(ch)->necromancer_bonus_levels;

  // Summoner stuff
  GET_EIDOLON_BASE_FORM(ch) = LEVELUP(ch)->eidolon_base_form;
  for (i = 1; i < NUM_EVOLUTIONS; i++)
  {
    KNOWS_EVOLUTION(ch, i) = LEVELUP(ch)->eidolon_evolutions[i];
    HAS_REAL_EVOLUTION(ch, i) = LEVELUP(ch)->summoner_aspects[i];
  }

  // Dragon Rider
  GET_DRAGON_RIDER_DRAGON_TYPE(ch) = LEVELUP(ch)->dragon_rider_dragon_type;
  GET_DRAGON_BOND_TYPE(ch) = LEVELUP(ch)->dragon_rider_bond_type;

  /* set spells learned for domain */
  assign_domain_spells(ch);

  /* make sure to disable restricted school spells */
  disable_restricted_school_spells(ch);

  /* in case adding or changing clear domains, clean up and re-assign */
  clear_domain_feats(ch);
  add_domain_feats(ch);
}

void use_boost_point(struct char_data *ch, int stat)
{

  struct descriptor_data *d = ch->desc;

  if (stat < 0 || stat > 6)
  {
    send_to_char(ch, "Error: Stat number out of bounds in use_boost_point.  Please report to staff.\r\n");
    mudlog(NRM, 31, TRUE, "Error: Stat number out of bounds in use_boost_point.  Please report to a coder.\r\n");
    return;
  }

  if (GET_LEVELUP_BOOST_STATS(ch, stat) > 0)
  {
    GET_LEVELUP_BOOST_STATS(ch, stat) = 0;
    GET_LEVELUP_BOOSTS(ch) = 1;
    main_boosts_disp_menu(d);
    send_to_char(ch, "You unassign your boost point from %s.\r\n", ability_score_names[stat]);
    return;
  }
  else
  {
    if (GET_LEVELUP_BOOSTS(ch) <= 0)
    {
      send_to_char(d->character, "You do not have any boost points to spend.\r\n");
      send_to_char(d->character, "Your choice (quit or ability score name) : ");
      return;
    }
    GET_LEVELUP_BOOST_STATS(ch, stat) = 1;
    GET_LEVELUP_BOOSTS(ch) = 0;
    main_boosts_disp_menu(d);
    send_to_char(ch, "You assign your boost point to %s.\r\n", ability_score_names[stat]);
    return;
  }
}

ACMD(do_study)
{
  /* gonna have some restrictions here */
  if (IS_WILDSHAPED(ch) || IS_MORPHED(ch))
  {
    send_to_char(ch, "You can not study while polymorphed, shape-changed or wildshaped.\r\n");
    return;
  }
  if (FIGHTING(ch))
  {
    send_to_char(ch, "Finish your battle first!\r\n");
    return;
  }
  if (IS_NPC(ch) || !ch->desc)
  { /* thanks fizban */
    send_to_char(ch, "Only PC's with a descriptor can study.\r\n");
    return;
  }

  struct descriptor_data *d = ch->desc;

  init_study(d, GET_CLASS(ch));

  act("$n starts adjusting $s skill-set.",
      TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

  // if (GET_LEVEL(ch) == 1 && stat_points_left(ch) > 0)  // old -- gicker, june 10, 2020
  if (GET_LEVEL(ch) == 1 && !HAS_SET_STATS_STUDY(ch) && GET_PREMADE_BUILD_CLASS(ch) == CLASS_UNDEFINED)
  {
    set_stats_menu(d);
    return;
  }

  display_main_menu(d);
}

bool add_levelup_feat(struct descriptor_data *d, int feat)
{
  struct char_data *ch = d->character;
  int feat_type = 0;

  if (feat <= FEAT_UNDEFINED || feat >= NUM_FEATS)
  {
    write_to_output(d, "Invalid feat.\r\n");
    return FALSE;
  }

  if (has_feat_requirement_check(ch, feat) && !feat_list[feat].can_stack)
  {
    write_to_output(d, "You already have this feat.\r\n");
    return FALSE;
  }

  if (feat_list[feat].epic == TRUE)
  { /* This is an epic feat! */
    if (is_class_feat(feat, LEVELUP(ch)->class, ch))
      feat_type = FEAT_TYPE_EPIC_CLASS;
    else
      feat_type = FEAT_TYPE_EPIC;
  }
  else if (feat_list[feat].teamwork_feat == TRUE)
  {
    feat_type = FEAT_TYPE_NORMAL_TEAMWORK;
  }
  else
  {
    if (is_class_feat(feat, LEVELUP(ch)->class, ch))
      feat_type = FEAT_TYPE_NORMAL_CLASS;
    else
      feat_type = FEAT_TYPE_NORMAL;
  }
  if (!isSorcBloodlineFeat(feat))
  {
    if ((feat_type == FEAT_TYPE_EPIC) && (LEVELUP(ch)->epic_feat_points < 1))
    {
      write_to_output(d, "You do not have enough epic feat points to gain that feat.\r\n");
      return FALSE;
    }
    if ((feat_type == FEAT_TYPE_EPIC_CLASS) &&
        ((LEVELUP(ch)->epic_feat_points < 1) &&
         (LEVELUP(ch)->epic_class_feat_points < 1)))
    {
      write_to_output(d, "You do not have enough epic class feat points to gain that feat.\r\n");
      return FALSE;
    }
    if ((feat_type == FEAT_TYPE_NORMAL_CLASS) &&
        ((LEVELUP(ch)->class_feat_points < 1) &&
         (LEVELUP(ch)->feat_points < 1) &&
         (LEVELUP(ch)->epic_class_feat_points < 1) &&
         (LEVELUP(ch)->epic_feat_points < 1)))
    {
      write_to_output(d, "You do not have enough class feat points to gain that feat.\r\n");
      return FALSE;
    }
    if ((feat_type == FEAT_TYPE_NORMAL_TEAMWORK) && (LEVELUP(ch)->feat_points < 1) && (LEVELUP(ch)->teamwork_feat_points < 1) &&
        (LEVELUP(ch)->epic_feat_points < 1))
    {
      write_to_output(d, "You do not have enough teamwork feat points to gain that feat.\r\n");
      return FALSE;
    }
    if ((feat_type == FEAT_TYPE_NORMAL) && (LEVELUP(ch)->feat_points < 1) &&
        (LEVELUP(ch)->epic_feat_points < 1))
    {
      write_to_output(d, "You do not have enough feat points to gain that feat.\r\n");
      return FALSE;
    }

    /* If we are here, then we can add the feat! */
    switch (feat_type)
    {
    case FEAT_TYPE_EPIC:
      LEVELUP(ch)->epic_feat_points--;
      break;
    case FEAT_TYPE_EPIC_CLASS:
      if (LEVELUP(ch)->epic_class_feat_points > 0)
        LEVELUP(ch)->epic_class_feat_points--;
      else
        LEVELUP(ch)->epic_feat_points--;
      break;
    case FEAT_TYPE_NORMAL_CLASS:
      if (LEVELUP(ch)->class_feat_points > 0)
        LEVELUP(ch)->class_feat_points--;
      else if (LEVELUP(ch)->feat_points > 0)
        LEVELUP(ch)->feat_points--;
      else if (LEVELUP(ch)->epic_class_feat_points > 0)
      {
        LEVELUP(ch)->epic_class_feat_points--;
        write_to_output(d, "You have used an epic class feat point to acquire a normal "
                           "class feat, if you do not want to do this, exit out of the study menu "
                           "without saving.\r\n");
      }
      else
      {
        LEVELUP(ch)->epic_feat_points--;
        write_to_output(d, "You have used an epic feat point to acquire a normal "
                           "class feat, if you do not want to do this, exit out of the study menu "
                           "without saving.\r\n");
      }
      break;
    case FEAT_TYPE_NORMAL_TEAMWORK:
      if (LEVELUP(ch)->teamwork_feat_points > 0)
        LEVELUP(ch)->teamwork_feat_points--;
      else if (LEVELUP(ch)->feat_points > 0)
        LEVELUP(ch)->feat_points--;
      else
      {
        LEVELUP(ch)->epic_feat_points--;
        write_to_output(d, "You have used an epic feat point to acquire a normal "
                           "teamwork feat, if you do not want to do this, exit out of the study menu "
                           "without saving.\r\n");
      }
      break;
    case FEAT_TYPE_NORMAL:
      if (LEVELUP(ch)->feat_points > 0)
        LEVELUP(ch)->feat_points--;
      else
      {
        LEVELUP(ch)->epic_feat_points--;
        write_to_output(d, "You have used an epic feat point to acquire a normal "
                           "feat, if you do not want to do this, exit out of the study menu "
                           "without saving.\r\n");
      }
      break;
    }
  }

  LEVELUP(ch)->feats[feat]++;
  return TRUE;
}

/*-------------------------------------------------------------------*/

/**************************************************************************
 Menu functions
 **************************************************************************/

/*-------------------------------------------------------------------*/

static void display_main_menu(struct descriptor_data *d)
{
  generic_main_disp_menu(d);
}

static void sorc_known_spells_disp_menu(struct descriptor_data *d)
{
  //  int sorc_level = CLASS_LEVEL(d->character, CLASS_SORCERER) +
  //                     BONUS_CASTER_LEVEL(d->character, CLASS_SORCERER);
  get_char_colors(d->character);
  clear_screen(d);

  char arcana_slots[100];
  if (free_arcana_slots(d->character) > 0)
    snprintf(arcana_slots, sizeof(arcana_slots), "%s A%s) Assign New Arcana Circle\r\n\r\n", grn, nrm);
  else
    snprintf(arcana_slots, sizeof(arcana_slots), "%s", nrm);

  write_to_output(d,
                  "\r\n-- %sSpells Known Menu\r\n"
                  "\r\n"
                  "%s 1%s) 1st Circle     : %s%d of %d selected\r\n"
                  "%s 2%s) 2nd Circle     : %s%d of %d selected\r\n"
                  "%s 3%s) 3rd Circle     : %s%d of %d selected\r\n"
                  "%s 4%s) 4th Circle     : %s%d of %d selected\r\n"
                  "%s 5%s) 5th Circle     : %s%d of %d selected\r\n"
                  "%s 6%s) 6th Circle     : %s%d of %d selected\r\n"
                  "%s 7%s) 7th Circle     : %s%d of %d selected\r\n"
                  "%s 8%s) 8th Circle     : %s%d of %d selected\r\n"
                  "%s 9%s) 9th Circle     : %s%d of %d selected\r\n"
                  "\r\n"
                  "%s"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "Enter Choice : ",
                  /*
                  mgn,
                  grn, nrm, yel, sorcerer_known[sorc_level][1] -
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 1),
                  grn, nrm, yel, sorcerer_known[sorc_level][2] -
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 2),
                  grn, nrm, yel, sorcerer_known[sorc_level][3] -
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 3),
                  grn, nrm, yel, sorcerer_known[sorc_level][4] -
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 4),
                  grn, nrm, yel, sorcerer_known[sorc_level][5] -
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 5),
                  grn, nrm, yel, sorcerer_known[sorc_level][6] -
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 6),
                  grn, nrm, yel, sorcerer_known[sorc_level][7] -
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 7),
                  grn, nrm, yel, sorcerer_known[sorc_level][8] -
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 8),
                  grn, nrm, yel, sorcerer_known[sorc_level][9] -
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 9),
                  arcana_slots,
                  grn, nrm
*/
                  mgn,
                  grn, nrm, yel,
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 1),
                  compute_slots_by_circle(d->character, CLASS_SORCERER, 1),
                  grn, nrm, yel,
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 2),
                  compute_slots_by_circle(d->character, CLASS_SORCERER, 2),
                  grn, nrm, yel,
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 3),
                  compute_slots_by_circle(d->character, CLASS_SORCERER, 3),
                  grn, nrm, yel,
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 4),
                  compute_slots_by_circle(d->character, CLASS_SORCERER, 4),
                  grn, nrm, yel,
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 5),
                  compute_slots_by_circle(d->character, CLASS_SORCERER, 5),
                  grn, nrm, yel,
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 6),
                  compute_slots_by_circle(d->character, CLASS_SORCERER, 6),
                  grn, nrm, yel,
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 7),
                  compute_slots_by_circle(d->character, CLASS_SORCERER, 7),
                  grn, nrm, yel,
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 8),
                  compute_slots_by_circle(d->character, CLASS_SORCERER, 8),
                  grn, nrm, yel,
                  count_known_spells_by_circle(d->character, CLASS_SORCERER, 9),
                  compute_slots_by_circle(d->character, CLASS_SORCERER, 9),
                  arcana_slots,
                  grn, nrm);

  OLC_MODE(d) = STUDY_SORC_KNOWN_SPELLS_MENU;
}

static void psionicist_known_powers_disp_menu(struct descriptor_data *d)
{
  //  int sorc_level = CLASS_LEVEL(d->character, CLASS_SORCERER) +
  //                     BONUS_CASTER_LEVEL(d->character, CLASS_SORCERER);
  get_char_colors(d->character);
  clear_screen(d);

  char arcana_slots[100];
  if (free_arcana_slots(d->character) > 0)
    snprintf(arcana_slots, sizeof(arcana_slots), "%s A%s) Assign New Arcana Circle\r\n\r\n", grn, nrm);
  else
    snprintf(arcana_slots, sizeof(arcana_slots), "%s", nrm);

  write_to_output(d,
                  "\r\n"
                  "-- %sPowers Known Menu\r\n"
                  "\r\n"
                  "%s 1%s) 1st Circle\r\n"
                  "%s 2%s) 2nd Circle\r\n"
                  "%s 3%s) 3rd Circle\r\n"
                  "%s 4%s) 4th Circle\r\n"
                  "%s 5%s) 5th Circle\r\n"
                  "%s 6%s) 6th Circle\r\n"
                  "%s 7%s) 7th Circle\r\n"
                  "%s 8%s) 8th Circle\r\n"
                  "%s 9%s) 9th Circle\r\n"
                  "\r\n"
                  "%s%d%s of %s%d%s powers selected.\r\n"
                  "\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "Enter Choice : ",

                  mgn,
                  grn, nrm,
                  grn, nrm,
                  grn, nrm,
                  grn, nrm,
                  grn, nrm,
                  grn, nrm,
                  grn, nrm,
                  grn, nrm,
                  grn, nrm,
                  yel, num_psionicist_powers_known(d->character), nrm, yel, num_psionicist_powers_available(d->character), nrm,
                  grn, nrm);

  OLC_MODE(d) = STUDY_PSIONICIST_KNOWN_POWERS_MENU;
}

void sorc_study_menu(struct descriptor_data *d, int circle)
{
  int counter, columns = 0;
  //  int class_level = CLASS_LEVEL(d->character, CLASS_SORCERER) +
  //                      BONUS_CASTER_LEVEL(d->character, CLASS_SORCERER);

  LEVELUP(d->character)->spell_circle = circle;

  get_char_colors(d->character);
  clear_screen(d);

  /* SPELL PREPARATION HOOK */
  for (counter = 1; counter < NUM_SPELLS; counter++)
  {
    if (compute_spells_circle(d->character, CLASS_SORCERER,
                              counter,
                              METAMAGIC_NONE,
                              DOMAIN_UNDEFINED) == circle)
    {
      if (is_a_known_spell(d->character, CLASS_SORCERER, counter))
        write_to_output(d, "%s%3d%s)%s+%-20.20s %s", grn, counter, nrm, mgn,
                        spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
      else
        write_to_output(d, "%s%3d%s) %s%-20.20s %s", grn, counter, nrm, yel,
                        spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
    }
  }
  write_to_output(d, "\r\n");
  write_to_output(d, "%sNumber of slots available:%s %d.\r\n", grn, nrm,
                  //          sorcerer_known[class_level][circle] -
                  compute_slots_by_circle(d->character, CLASS_SORCERER, circle) -
                      count_known_spells_by_circle(d->character, CLASS_SORCERER, circle));
  write_to_output(d, "\tCType the spell number followed by 'help' to see help on that spell.  Eg. 81 help\r\n\tn");
  write_to_output(d, "%s+ A plus sign marks your current selection(s).\r\n", nrm);
  write_to_output(d, "%sEnter spell choice, to add or remove "
                     "(Q to exit to main menu) : ",
                  nrm);

  OLC_MODE(d) = STUDY_SPELLS;
}

void warlock_known_spells_disp_menu(struct descriptor_data *d)
{
  int class_level = CLASS_LEVEL(d->character, CLASS_WARLOCK) +
                    BONUS_CASTER_LEVEL(d->character, CLASS_WARLOCK);

  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sInvocations Known Menu\r\n"
                  "\r\n"
                  "%s 1%s) Least      : %s%d\r\n"
                  "%s 2%s) Lesser     : %s%d\r\n"
                  "%s 3%s) Greater    : %s%d\r\n"
                  "%s 4%s) Dark       : %s%d\r\n"
                  "\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "Enter Choice : ",

                  mgn,
                  grn, nrm, yel, warlock_known[class_level][1] - count_known_spells_by_circle(d->character, CLASS_WARLOCK, 1),
                  grn, nrm, yel, warlock_known[class_level][2] - count_known_spells_by_circle(d->character, CLASS_WARLOCK, 2),
                  grn, nrm, yel, warlock_known[class_level][3] - count_known_spells_by_circle(d->character, CLASS_WARLOCK, 3),
                  grn, nrm, yel, warlock_known[class_level][4] - count_known_spells_by_circle(d->character, CLASS_WARLOCK, 4),
                  grn, nrm);

  OLC_MODE(d) = STUDY_WARLOCK_KNOWN_SPELLS_MENU;
}

static void bard_known_spells_disp_menu(struct descriptor_data *d)
{
  int class_level = CLASS_LEVEL(d->character, CLASS_BARD) +
                    BONUS_CASTER_LEVEL(d->character, CLASS_BARD);

  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sSpells Known Menu\r\n"
                  "\r\n"
                  "%s 1%s) 1st Circle     : %s%d\r\n"
                  "%s 2%s) 2nd Circle     : %s%d\r\n"
                  "%s 3%s) 3rd Circle     : %s%d\r\n"
                  "%s 4%s) 4th Circle     : %s%d\r\n"
                  "%s 5%s) 5th Circle     : %s%d\r\n"
                  "%s 6%s) 6th Circle     : %s%d\r\n"
                  "\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "Enter Choice : ",

                  mgn,
                  grn, nrm, yel, bard_known[class_level][1] - count_known_spells_by_circle(d->character, CLASS_BARD, 1),
                  grn, nrm, yel, bard_known[class_level][2] - count_known_spells_by_circle(d->character, CLASS_BARD, 2),
                  grn, nrm, yel, bard_known[class_level][3] - count_known_spells_by_circle(d->character, CLASS_BARD, 3),
                  grn, nrm, yel, bard_known[class_level][4] - count_known_spells_by_circle(d->character, CLASS_BARD, 4),
                  grn, nrm, yel, bard_known[class_level][5] - count_known_spells_by_circle(d->character, CLASS_BARD, 5),
                  grn, nrm, yel, bard_known[class_level][6] - count_known_spells_by_circle(d->character, CLASS_BARD, 6),
                  grn, nrm);

  OLC_MODE(d) = STUDY_BARD_KNOWN_SPELLS_MENU;
}


static void summoner_known_spells_disp_menu(struct descriptor_data *d)
{
  int class_level = CLASS_LEVEL(d->character, CLASS_SUMMONER) +
                    BONUS_CASTER_LEVEL(d->character, CLASS_SUMMONER);

  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sSpells Known Menu\r\n"
                  "\r\n"
                  "%s 1%s) 1st Circle     : %s%d\r\n"
                  "%s 2%s) 2nd Circle     : %s%d\r\n"
                  "%s 3%s) 3rd Circle     : %s%d\r\n"
                  "%s 4%s) 4th Circle     : %s%d\r\n"
                  "%s 5%s) 5th Circle     : %s%d\r\n"
                  "%s 6%s) 6th Circle     : %s%d\r\n"
                  "\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "Enter Choice : ",

                  mgn,
                  grn, nrm, yel, bard_known[class_level][1] - count_known_spells_by_circle(d->character, CLASS_SUMMONER, 1),
                  grn, nrm, yel, bard_known[class_level][2] - count_known_spells_by_circle(d->character, CLASS_SUMMONER, 2),
                  grn, nrm, yel, bard_known[class_level][3] - count_known_spells_by_circle(d->character, CLASS_SUMMONER, 3),
                  grn, nrm, yel, bard_known[class_level][4] - count_known_spells_by_circle(d->character, CLASS_SUMMONER, 4),
                  grn, nrm, yel, bard_known[class_level][5] - count_known_spells_by_circle(d->character, CLASS_SUMMONER, 5),
                  grn, nrm, yel, bard_known[class_level][6] - count_known_spells_by_circle(d->character, CLASS_SUMMONER, 6),
                  grn, nrm);

  OLC_MODE(d) = STUDY_SUMMONER_KNOWN_SPELLS_MENU;
}

static void inquisitor_known_spells_disp_menu(struct descriptor_data *d)
{
  int class_level = CLASS_LEVEL(d->character, CLASS_INQUISITOR) +
                    BONUS_CASTER_LEVEL(d->character, CLASS_INQUISITOR);

  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sSpells Known Menu\r\n"
                  "\r\n"
                  "%s 1%s) 1st Circle     : %s%d\r\n"
                  "%s 2%s) 2nd Circle     : %s%d\r\n"
                  "%s 3%s) 3rd Circle     : %s%d\r\n"
                  "%s 4%s) 4th Circle     : %s%d\r\n"
                  "%s 5%s) 5th Circle     : %s%d\r\n"
                  "%s 6%s) 6th Circle     : %s%d\r\n"
                  "\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "Enter Choice : ",

                  mgn,
                  grn, nrm, yel, inquisitor_known[class_level][1] - count_known_spells_by_circle(d->character, CLASS_INQUISITOR, 1),
                  grn, nrm, yel, inquisitor_known[class_level][2] - count_known_spells_by_circle(d->character, CLASS_INQUISITOR, 2),
                  grn, nrm, yel, inquisitor_known[class_level][3] - count_known_spells_by_circle(d->character, CLASS_INQUISITOR, 3),
                  grn, nrm, yel, inquisitor_known[class_level][4] - count_known_spells_by_circle(d->character, CLASS_INQUISITOR, 4),
                  grn, nrm, yel, inquisitor_known[class_level][5] - count_known_spells_by_circle(d->character, CLASS_INQUISITOR, 5),
                  grn, nrm, yel, inquisitor_known[class_level][6] - count_known_spells_by_circle(d->character, CLASS_INQUISITOR, 6),
                  grn, nrm);

  OLC_MODE(d) = STUDY_INQUISITOR_KNOWN_SPELLS_MENU;
}

/* the menu for each circle, inquisitor */
void inquisitor_study_menu(struct descriptor_data *d, int circle)
{
  int class_level = CLASS_LEVEL(d->character, CLASS_INQUISITOR) +
                    BONUS_CASTER_LEVEL(d->character, CLASS_INQUISITOR);
  int counter, columns = 0;

  LEVELUP(d->character)->spell_circle = circle;

  get_char_colors(d->character);
  clear_screen(d);

  /* SPELL PREPARATION HOOK */
  for (counter = 1; counter < NUM_SPELLS; counter++)
  {
    if (compute_spells_circle(d->character, CLASS_INQUISITOR,
                              counter,
                              METAMAGIC_NONE,
                              DOMAIN_UNDEFINED) == circle)
    {
      if (is_a_known_spell(d->character, CLASS_INQUISITOR, counter))
        write_to_output(d, "%s%3d%s) %s+%-30.30s %s", grn, counter, nrm, mgn,
                        spell_info[counter].name,
                        !(++columns % 3) ? "\r\n" : "");
      else
        write_to_output(d, "%s%3d%s) %s %-30.30s %s", grn, counter, nrm, yel,
                        spell_info[counter].name,
                        !(++columns % 3) ? "\r\n" : "");
    }
  }
  write_to_output(d, "\r\n");
  write_to_output(d, "%sNumber of slots available:%s %d.\r\n", grn, nrm,
                  inquisitor_known[class_level][circle] -
                      count_known_spells_by_circle(d->character, CLASS_INQUISITOR, circle));
  write_to_output(d, "\tCType the spell number followed by 'help' to see help on that spell.  Eg. 81 help\r\n\tn");
  write_to_output(d, "%s+ A plus sign marks your current selection(s).\r\n"
                     "Enter spell choice, to add or remove (Q to exit to main menu) : ",
                  nrm);

  OLC_MODE(d) = INQUISITOR_STUDY_SPELLS;
}

/* the menu for each circle, warlock */
void warlock_study_menu(struct descriptor_data *d, int circle)
{
  int class_level = CLASS_LEVEL(d->character, CLASS_WARLOCK) +
                    BONUS_CASTER_LEVEL(d->character, CLASS_WARLOCK);
  int counter, columns = 0;

  LEVELUP(d->character)->spell_circle = circle;

  get_char_colors(d->character);
  clear_screen(d);

  /* SPELL PREPARATION HOOK */
  for (counter = WARLOCK_POWER_START + 2; counter < WARLOCK_POWER_END; counter++)
  {    
    if (compute_spells_circle(d->character, CLASS_WARLOCK,
                              counter,
                              METAMAGIC_NONE,
                              DOMAIN_UNDEFINED) == circle)
    {
      if (warlock_spell_type(counter) == WARLOCK_POWER_NONE) continue;

      if (is_a_known_spell(d->character, CLASS_WARLOCK, counter))
        write_to_output(d, "%s%2d%s) %s+%-20.20s %s", grn, counter, nrm, mgn,
                        spell_info[counter].name,
                        !(++columns % 3) ? "\r\n" : "");
      else
        write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
                        spell_info[counter].name,
                        !(++columns % 3) ? "\r\n" : "");
    }
  }
  write_to_output(d, "\r\n");
  write_to_output(d, "%sNumber of slots available:%s %d.\r\n", grn, nrm,
                  warlock_known[class_level][circle] -
                      count_known_spells_by_circle(d->character, CLASS_WARLOCK, circle));
  write_to_output(d, "\tCType the invocation number followed by 'help' to see help on that invocation.  Eg. 81 help\r\n\tn");
  write_to_output(d, "%s+ A plus sign marks your current selection(s).\r\n"
                     "Enter spell choice, to add or remove (Q to exit to main menu) : ",
                  nrm);

  OLC_MODE(d) = WARLOCK_STUDY_SPELLS;
}

/* the menu for each circle, bard */
void bard_study_menu(struct descriptor_data *d, int circle)
{
  int class_level = CLASS_LEVEL(d->character, CLASS_BARD) +
                    BONUS_CASTER_LEVEL(d->character, CLASS_BARD);
  int counter, columns = 0;

  LEVELUP(d->character)->spell_circle = circle;

  get_char_colors(d->character);
  clear_screen(d);

  /* SPELL PREPARATION HOOK */
  for (counter = 1; counter < NUM_SPELLS; counter++)
  {
    if (compute_spells_circle(d->character, CLASS_BARD,
                              counter,
                              METAMAGIC_NONE,
                              DOMAIN_UNDEFINED) == circle)
    {
      if (is_a_known_spell(d->character, CLASS_BARD, counter))
        write_to_output(d, "%s%3d%s) %s+%-20.20s %s", grn, counter, nrm, mgn,
                        spell_info[counter].name,
                        !(++columns % 3) ? "\r\n" : "");
      else
        write_to_output(d, "%s%3d%s) %s%-20.20s %s", grn, counter, nrm, yel,
                        spell_info[counter].name,
                        !(++columns % 3) ? "\r\n" : "");
    }
  }
  write_to_output(d, "\r\n");
  write_to_output(d, "%sNumber of slots available:%s %d.\r\n", grn, nrm,
                  bard_known[class_level][circle] -
                      count_known_spells_by_circle(d->character, CLASS_BARD, circle));
  write_to_output(d, "\tCType the spell number followed by 'help' to see help on that spell.  Eg. 81 help\r\n\tn");
  write_to_output(d, "%s+ A plus sign marks your current selection(s).\r\n"
                     "Enter spell choice, to add or remove (Q to exit to main menu) : ",
                  nrm);

  OLC_MODE(d) = BARD_STUDY_SPELLS;
}

/* the menu for each circle, bard */
void summoner_study_menu(struct descriptor_data *d, int circle)
{
  int class_level = CLASS_LEVEL(d->character, CLASS_SUMMONER) + BONUS_CASTER_LEVEL(d->character, CLASS_SUMMONER);
  int counter, columns = 0;

  LEVELUP(d->character)->spell_circle = circle;

  get_char_colors(d->character);
  clear_screen(d);

  /* SPELL PREPARATION HOOK */
  for (counter = 1; counter < NUM_SPELLS; counter++)
  {
    if (compute_spells_circle(d->character, CLASS_SUMMONER,
                              counter,
                              METAMAGIC_NONE,
                              DOMAIN_UNDEFINED) == circle)
    {
      if (is_a_known_spell(d->character, CLASS_SUMMONER, counter))
        write_to_output(d, "%s%3d%s) %s+%-20.20s %s", grn, counter, nrm, mgn,
                        spell_info[counter].name,
                        !(++columns % 3) ? "\r\n" : "");
      else
        write_to_output(d, "%s%3d%s) %s%-20.20s %s", grn, counter, nrm, yel,
                        spell_info[counter].name,
                        !(++columns % 3) ? "\r\n" : "");
    }
  }
  write_to_output(d, "\r\n");
  write_to_output(d, "%sNumber of slots available:%s %d.\r\n", grn, nrm,
                  summoner_known[class_level][circle] - count_known_spells_by_circle(d->character, CLASS_SUMMONER, circle));
  write_to_output(d, "\tCType the spell number followed by 'help' to see help on that spell.  Eg. 81 help\r\n\tn");
  write_to_output(d, "%s+ A plus sign marks your current selection(s).\r\n"
                     "Enter spell choice, to add or remove (Q to exit to main menu) : ",
                  nrm);

  OLC_MODE(d) = SUMMONER_STUDY_SPELLS;
}

void psionicist_study_menu(struct descriptor_data *d, int circle)
{
  int counter, columns = 0;

  LEVELUP(d->character)->spell_circle = circle;

  get_char_colors(d->character);
  clear_screen(d);

  /* SPELL PREPARATION HOOK */
  for (counter = PSIONIC_POWER_START; counter <= PSIONIC_POWER_END; counter++)
  {
    if (compute_powers_circle(CLASS_PSIONICIST, counter, METAMAGIC_NONE) == circle)
    {
      if (is_a_known_spell(d->character, CLASS_PSIONICIST, counter))
        write_to_output(d, "%s%2d%s)%s+%-25.25s %s", grn, counter, nrm, mgn,
                        spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
      else
        write_to_output(d, "%s%2d%s) %s%-25.25s %s", grn, counter, nrm, yel,
                        spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
    }
  }
  write_to_output(d, "\r\n");
  write_to_output(d, "%sNumber of slots available:%s %d.\r\n", grn, nrm,
                  num_psionicist_powers_available(d->character) - num_psionicist_powers_known(d->character));
  write_to_output(d, "\tCType the power number followed by 'help' to see help on that power.  Eg. 81 help\r\n\tn");
  write_to_output(d, "%s+ A plus sign marks your current selection(s).\r\n", nrm);
  write_to_output(d, "%sEnter power choice, to add or remove "
                     "(Q to exit to main menu) : ",
                  nrm);

  OLC_MODE(d) = PSIONICIST_STUDY_POWERS;
}

/***********************end bard******************************************/

static void favored_enemy_submenu(struct descriptor_data *d, int favored)
{
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sRanger Favored Enemy Sub-Menu%s\r\n"
                  "Slot:  %d\r\n"
                  "\r\n"
                  "%s"
                  "\r\n"
                  "Current Favored Enemy:  %s\r\n"
                  "You can select 0 (Zero) to deselect an enemy for this slot.\r\n"
                  "\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "Enter Choice : ",

                  mgn, nrm,
                  favored,
                  /* empty line */
                  npc_race_menu,
                  /* empty line */
                  race_family_abbrevs[GET_FAVORED_ENEMY(d->character, favored)],
                  grn, nrm
                  /* empty line */
  );

  OLC_MODE(d) = FAVORED_ENEMY_SUB;
}

int stat_cost_chart[11] = {/* cost for total points */
                           /*0  1  2  3  4  5  6  7  8   9   10 */
                           0, 1, 2, 3, 4, 5, 6, 8, 10, 13, 16};
int compute_base_dex(struct char_data *ch)
{
  int base_dex = BASE_STAT;
  /*
  switch (GET_RACE(ch))
  {
  case RACE_ELF:
    base_dex += 2;
    break;
  case RACE_DROW:
    base_dex += 2;
    break;
  case RACE_HALFLING:
    base_dex += 2;
    break;
  case RACE_HALF_TROLL:
    base_dex += 2;
    break;
  case RACE_TRELUX:
    base_dex += 8;
    break;
  }
  */
  return base_dex;
}
int compute_dex_cost(struct char_data *ch, int number)
{
  int base_dex = compute_base_dex(ch), current_dex = LEVELUP(ch)->dex + number;
  return stat_cost_chart[current_dex - base_dex];
}
int compute_base_str(struct char_data *ch)
{
  int base_str = BASE_STAT;
  /*
  switch (GET_RACE(ch))
  {
  case RACE_HALFLING:
    base_str -= 2;
    break;
  case RACE_GNOME:
    base_str -= 2;
    break;
  case RACE_HALF_TROLL:
    base_str += 2;
    break;
  case RACE_CRYSTAL_DWARF:
    base_str += 2;
    break;
  case RACE_TRELUX:
    base_str += 2;
    break;
  case RACE_ARCANA_GOLEM:
    base_str -= 2;
    break;
  case RACE_HALF_ORC:
    base_str += 2;
    break;
  }
  */
  return base_str;
}
int compute_str_cost(struct char_data *ch, int number)
{
  int base_str = compute_base_str(ch),
      current_str = LEVELUP(ch)->str + number;
  return stat_cost_chart[current_str - base_str];
}
int compute_base_con(struct char_data *ch)
{
  int base_con = BASE_STAT;
  /*
  switch (GET_RACE(ch))
  {
  case RACE_ELF:
    base_con -= 2;
    break;
  case RACE_DROW:
    base_con -= 2;
    break;
  case RACE_DWARF:
    base_con += 2;
    break;
  case RACE_GNOME:
    base_con += 2;
    break;
  case RACE_HALF_TROLL:
    base_con += 2;
    break;
  case RACE_CRYSTAL_DWARF:
    base_con += 8;
    break;
  case RACE_TRELUX:
    base_con += 4;
    break;
  case RACE_ARCANA_GOLEM:
    base_con -= 2;
    break;
  }
  */
  return base_con;
}
int compute_con_cost(struct char_data *ch, int number)
{
  int base_con = compute_base_con(ch), current_con = LEVELUP(ch)->con + number;
  return stat_cost_chart[current_con - base_con];
}
int compute_base_inte(struct char_data *ch)
{
  int base_inte = BASE_STAT;
  /*
  switch (GET_RACE(ch))
  {
  case RACE_HALF_TROLL:
    base_inte -= 4;
    break;
  case RACE_ARCANA_GOLEM:
    base_inte += 2;
    break;
  case RACE_DROW:
    base_inte += 2;
    break;
  case RACE_HALF_ORC:
    base_inte -= 2;
    break;
  }
  */
  return base_inte;
}
int compute_inte_cost(struct char_data *ch, int number)
{
  int base_inte = compute_base_inte(ch), current_inte = LEVELUP(ch)->inte + number;
  return stat_cost_chart[current_inte - base_inte];
}
int compute_base_wis(struct char_data *ch)
{
  int base_wis = BASE_STAT;
  /*
  switch (GET_RACE(ch))
  {
  case RACE_HALF_TROLL:
    base_wis -= 4;
    break;
  case RACE_CRYSTAL_DWARF:
    base_wis += 2;
    break;
  case RACE_ARCANA_GOLEM:
    base_wis += 2;
    break;
  case RACE_DROW:
    base_wis += 2;
    break;
  }
  */
  return base_wis;
}
int compute_wis_cost(struct char_data *ch, int number)
{
  int base_wis = compute_base_wis(ch), current_wis = LEVELUP(ch)->wis + number;
  return stat_cost_chart[current_wis - base_wis];
}
int compute_base_cha(struct char_data *ch)
{
  int base_cha = BASE_STAT;
  /*
  switch (GET_RACE(ch))
  {
  case RACE_DWARF:
    base_cha -= 2;
    break;
  case RACE_DROW:
    base_cha += 2;
    break;
  case RACE_HALF_TROLL:
    base_cha -= 4;
    break;
  case RACE_CRYSTAL_DWARF:
    base_cha += 2;
    break;
  case RACE_ARCANA_GOLEM:
    base_cha += 2;
    break;
  case RACE_HALF_ORC:
    base_cha -= 2;
    break;
  }
  */
  return base_cha;
}
int compute_cha_cost(struct char_data *ch, int number)
{
  int base_cha = compute_base_cha(ch), current_cha = LEVELUP(ch)->cha + number;
  return stat_cost_chart[current_cha - base_cha];
}
int compute_total_stat_points(struct char_data *ch)
{
  return (compute_cha_cost(ch, 0) + compute_wis_cost(ch, 0) + compute_inte_cost(ch, 0) +
          compute_str_cost(ch, 0) + compute_dex_cost(ch, 0) + compute_con_cost(ch, 0));
}
int stat_points_left(struct char_data *ch)
{
  if (GET_PREMADE_BUILD_CLASS(ch) >= 0)
    return 0;
  return (TOTAL_STAT_POINTS(ch) - compute_total_stat_points(ch));
}
static void set_stats_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sSet Character Stats%s\r\n"
                  "From this menu you can set your starting stats.  If you are completely\r\n"
                  "unfamiliar with what type of stats you want, it is highly recommended\r\n"
                  "to keep all values at or above 10, and spread out your points.  This choice\r\n"
                  "will give you access to the most feats possible regardless of class.\r\n"
                  "You will be required to use all your points before advancing your char.\r\n"
                  "As you get more familiar with the feat system, there are opportunities\r\n"
                  "to reset your characters stats to try a different configuration.\r\n"
                  "Change made to base stat:   1  2  3  4  5  6  7  8   9   10\r\n"
                  "Point cost              :   1  2  3  4  5  6  8  10  13  16\r\n"
                  "                 Base | Race Mod | Final\r\n"
                  "%s 0%s) Strength:      %2d%s | %6s%s%d | %d\r\n"
                  "%s 1%s) Dexterity:     %2d%s | %6s%s%d | %d\r\n"
                  "%s 2%s) Constitution:  %2d%s | %6s%s%d | %d\r\n"
                  "%s 3%s) Intelligence:  %2d%s | %6s%s%d | %d\r\n"
                  "%s 4%s) Wisdom:        %2d%s | %6s%s%d | %d\r\n"
                  "%s 5%s) Charisma:      %2d%s | %6s%s%d | %d\r\n"
                  "%sPoints Left:         %d%s\r\n"
                  "\r\n"
                  "%s H%s) Help - Info on What Each Ability Score Does.\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "To set up your base ability scores type the number to the left of the ability score \r\n"
                  "you wish to modify and press enter, then type the number of points you wish to invest \r\n"
                  "or remove as a digit.\r\n"
                  "EG. 0 <ENTER> 4 <ENTER>\r\n"
                  "This example willl add 4 points to your strength score.  Note that higher ability scores \r\n"
                  "use up more points as per the table above.\r\n"
                  "\r\n"
                  "Enter Choice : ",

                  mgn, nrm,
                  /* empty line */
                  grn, nrm, LEVELUP(d->character)->str, nrm, "", get_race_stat(GET_RACE(d->character), R_STR_MOD) >= 0 ? "+" : "",
                  get_race_stat(GET_RACE(d->character), R_STR_MOD), get_race_stat(GET_RACE(d->character), R_STR_MOD) + LEVELUP(d->character)->str,
                  grn, nrm, LEVELUP(d->character)->dex, nrm, "", get_race_stat(GET_RACE(d->character), R_DEX_MOD) >= 0 ? "+" : "",
                  get_race_stat(GET_RACE(d->character), R_DEX_MOD), get_race_stat(GET_RACE(d->character), R_DEX_MOD) + LEVELUP(d->character)->dex,
                  grn, nrm, LEVELUP(d->character)->con, nrm, "", get_race_stat(GET_RACE(d->character), R_CON_MOD) >= 0 ? "+" : "",
                  get_race_stat(GET_RACE(d->character), R_CON_MOD), get_race_stat(GET_RACE(d->character), R_CON_MOD) + LEVELUP(d->character)->con,
                  grn, nrm, LEVELUP(d->character)->inte, nrm, "", get_race_stat(GET_RACE(d->character), R_INTEL_MOD) >= 0 ? "+" : "",
                  get_race_stat(GET_RACE(d->character), R_INTEL_MOD), get_race_stat(GET_RACE(d->character), R_INTEL_MOD) + LEVELUP(d->character)->inte,
                  grn, nrm, LEVELUP(d->character)->wis, nrm, "", get_race_stat(GET_RACE(d->character), R_WIS_MOD) >= 0 ? "+" : "",
                  get_race_stat(GET_RACE(d->character), R_WIS_MOD), get_race_stat(GET_RACE(d->character), R_WIS_MOD) + LEVELUP(d->character)->wis,
                  grn, nrm, LEVELUP(d->character)->cha, nrm, "", get_race_stat(GET_RACE(d->character), R_CHA_MOD) >= 0 ? "+" : "",
                  get_race_stat(GET_RACE(d->character), R_CHA_MOD), get_race_stat(GET_RACE(d->character), R_CHA_MOD) + LEVELUP(d->character)->cha,
                  grn, stat_points_left(d->character), nrm,
                  /* empty line */
                  grn, nrm,
                  /* empty line */
                  grn, nrm
                  /* empty line */
  );

  OLC_MODE(d) = STUDY_SET_STATS;
}

static void set_school_submenu(struct descriptor_data *d)
{
  int i;

  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d, "\r\n");
  for (i = 0; i < NUM_SCHOOLS; i++)
  {
    write_to_output(d, "%d) %s\r\n", i, school_names[i]);
  }
  write_to_output(d, "\r\n");

  write_to_output(d, "\r\n%sEnter magical art to specialize in : ", nrm);
}
static void set_school_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sSet School of Magic Specialty%s\r\n"
                  "\r\n"
                  "%s 0%s) Specialty School:      %s%s\r\n"
                  "%s    Restricted School:     %s%s\r\n"
                  "\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "Enter Choice : ",

                  mgn, nrm,
                  /* empty line */
                  grn, nrm, school_names[GET_SPECIALTY_SCHOOL(d->character)], nrm,
                  nrm, school_names[restricted_school_reference[GET_SPECIALTY_SCHOOL(d->character)]], nrm,
                  /* empty line */
                  grn, nrm
                  /* empty line */
  );

  OLC_MODE(d) = STUDY_SET_SCHOOL;
}

static void set_preferred_arcane(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d, "\r\n");
  write_to_output(d, "%d) %s\r\n", CLASS_WIZARD, CLSLIST_NAME(CLASS_WIZARD));
  write_to_output(d, "%d) %s\r\n", CLASS_SORCERER, CLSLIST_NAME(CLASS_SORCERER));
  write_to_output(d, "%d) %s\r\n", CLASS_BARD, CLSLIST_NAME(CLASS_BARD));
  write_to_output(d, "%d) %s\r\n", CLASS_SUMMONER, CLSLIST_NAME(CLASS_SUMMONER));
  write_to_output(d, "\r\n");
  write_to_output(d, "\r\n%sEnter your preferred arcane class : ", nrm);
}
static void set_preferred_divine(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d, "\r\n");
  write_to_output(d, "%d) %s\r\n", CLASS_CLERIC, CLSLIST_NAME(CLASS_CLERIC));
  write_to_output(d, "%d) %s\r\n", CLASS_INQUISITOR, CLSLIST_NAME(CLASS_INQUISITOR));
  write_to_output(d, "%d) %s\r\n", CLASS_DRUID, CLSLIST_NAME(CLASS_DRUID));
  write_to_output(d, "\r\n");
  write_to_output(d, "\r\n%sEnter your preferred divine class : ", nrm);
}

static void set_bloodline_draconic(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d, "\r\n");
  write_to_output(d, "Your draconic heritage determines the damage type for your\r\n");
  write_to_output(d, "breath weapon and energy resistance once acquired.\r\n");
  write_to_output(d, "\r\n");
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_BLACK, DRCHRTLIST_NAME(DRACONIC_HERITAGE_BLACK), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_BLACK));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_BLUE, DRCHRTLIST_NAME(DRACONIC_HERITAGE_BLUE), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_BLUE));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_GREEN, DRCHRTLIST_NAME(DRACONIC_HERITAGE_GREEN), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_GREEN));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_RED, DRCHRTLIST_NAME(DRACONIC_HERITAGE_RED), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_RED));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_WHITE, DRCHRTLIST_NAME(DRACONIC_HERITAGE_WHITE), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_WHITE));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_BRASS, DRCHRTLIST_NAME(DRACONIC_HERITAGE_BRASS), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_BRASS));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_BRONZE, DRCHRTLIST_NAME(DRACONIC_HERITAGE_BRONZE), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_BRONZE));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_COPPER, DRCHRTLIST_NAME(DRACONIC_HERITAGE_COPPER), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_COPPER));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_SILVER, DRCHRTLIST_NAME(DRACONIC_HERITAGE_SILVER), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_SILVER));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_GOLD, DRCHRTLIST_NAME(DRACONIC_HERITAGE_GOLD), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_GOLD));
  write_to_output(d, "\r\n");
  write_to_output(d, "\r\n%sSelect the dragon type for your draconic heritage : ", nrm);
}

static void set_bloodline_arcane(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d, "\r\n");
  write_to_output(d, "Your arcane heritage mainly determines the spell school associated with\r\n");
  write_to_output(d, "with the 'school power' bloodline feat.\r\n");
  write_to_output(d, "\r\n");
  int i = 1;
  for (i; i < NUM_SCHOOLS; i++)
    write_to_output(d, "%d) %s\r\n", i, spell_schools[i]);
  write_to_output(d, "\r\n");
  write_to_output(d, "\r\n%sSelect the arcane school type for your arcane bloodline : ", nrm);
}

static void display_new_arcana_menu(struct descriptor_data *d)
{
  struct char_data *ch = d->character;
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n"
                  "New Arcana Menu\r\n"
                  "\r\n"
                  "Free NewArcana Slots: %s%d%s\r\n"
                  "\r\n"
                  "Please type in the number of the spell circle for which you would like\r\n"
                  "to receive a bonus spell choice.  For example if you would like to have an\r\n"
                  "extra circle 2 slot added to the list of spells you can case, then simply type\r\n"
                  "in '2' (without the quotes).  Once spent, these can only be recovered with a\r\n"
                  "respec, so choose carefully.  It also must be of a spell circle you are able\r\n"
                  "to cast.\r\n"
                  "\r\n"
                  "Enter Choice : ",
                  yel, free_arcana_slots(ch), nrm);

  OLC_MODE(d) = SET_NEW_ARCANA;
}

static void set_preferred_caster(struct descriptor_data *d)
{
  /* dummy checks :( */
  if (!d)
    return;
  if (!d->character)
    return;
  if (GET_PREFERRED_ARCANE(d->character) >= NUM_CLASSES || GET_PREFERRED_ARCANE(d->character) <= CLASS_UNDEFINED)
  {
    GET_PREFERRED_ARCANE(d->character) = CLASS_WIZARD;
    write_to_output(d, "There was an error, reset to wizard and exiting for you to try again!\r\n\r\n");
    return;
  }
  if (GET_PREFERRED_DIVINE(d->character) >= NUM_CLASSES || GET_PREFERRED_DIVINE(d->character) <= CLASS_UNDEFINED)
  {
    GET_PREFERRED_DIVINE(d->character) = CLASS_CLERIC;
    write_to_output(d, "There was an error, reset to cleric and exiting for you to try again!\r\n\r\n");
    return;
  }

  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sSet Preferred Caster Class (for Prestige Classes)%s\r\n"
                  "\r\n"
                  "%s 0%s) Arcane:      %s%s\r\n"
                  "%s 1%s) Divine:      %s%s\r\n"
                  "\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "Enter Choice : ",

                  mgn, nrm,
                  /* empty line */
                  grn, nrm, CLSLIST_NAME(GET_PREFERRED_ARCANE(d->character)), nrm,
                  grn, nrm, CLSLIST_NAME(GET_PREFERRED_DIVINE(d->character)), nrm,
                  /* empty line */
                  grn, nrm
                  /* empty line */
  );

  OLC_MODE(d) = STUDY_SET_P_CASTER;
}

bool has_necromancer_cast_type_unchosen(struct char_data *ch)
{
  if (!CLASS_LEVEL(ch, CLASS_NECROMANCER))
    return FALSE;
  return (LEVELUP(ch)->necromancer_bonus_levels == 0);
}

char *levelup_show_necromancer_cast_type(struct char_data *ch)
{
  switch (LEVELUP(ch)->necromancer_bonus_levels)
  {
    case 1: return "Arcane";
    case 2: return "Divine";
  }
  return "Not Chosen";
}

static void set_sorcerer_bloodline(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sSet Sorcerer Bloodline%s\r\n"
                  "\r\n"
                  "%s 0%s) Arcane\r\n"
                  "%s 1%s) Draconic\r\n"
                  "%s 2%s) Fey\r\n"
                  "%s 3%s) Undead\r\n"
                  "\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "Enter Choice : ",

                  mgn, nrm,
                  /* empty line */
                  grn, nrm,
                  /* empty line */
                  grn, nrm,
                  /* empty line */
                  grn, nrm,
                  /* empty line */
                  grn, nrm,
                  /* empty line */
                  grn, nrm
                  /* empty line */
  );

  OLC_MODE(d) = STUDY_SET_S_BLOODLINE;
}

static void select_racial_abils(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sSelect Racial Ability%s\r\n"
                  "\r\n"
                  "%s 0%s) High Elf Cantrip\r\n"
                  "%s 1%s) Dragonborn Dragon Ancestry\r\n"
                  "\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "Enter Choice : ",

                  mgn, nrm,
                  /* empty line */
                  MENU_OPT(!CAN_CHOOSE_HIGH_ELF_CANTRIP(d->character)),
                  /* empty line */
                  MENU_OPT(!CAN_CHOOSE_DRAGONBORN_ANCESTRY(d->character)),
                  /* empty line */
                  grn, nrm);

  OLC_MODE(d) = STUDY_SELECT_RACIAL_ABILITY;
}

static void select_alchemist_discoveries(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sSelect Alchemical Discovery%s\r\n"
                  "\r\n",
                  mgn, nrm);
  int i = 0;
  for (i = 1; i < NUM_ALC_DISCOVERIES; i++)
  {
    if ((i < 1) ||
        (i >= NUM_ALC_DISCOVERIES) ||
        (KNOWS_DISCOVERY(d->character, i)) ||
        (LEVELUP(d->character)->discoveries[i]) ||
        (!can_learn_discovery(d->character, i)))
      continue;
    write_to_output(d, "%s %2d%s) %s\r\n",
                    grn, i, nrm, alchemical_discovery_names[i]);
  }

  write_to_output(d, "\r\n"
                     "%s -1%s) Quit\r\n"
                     "\r\n"
                     "Enter Choice : ",
                  grn, nrm);

  OLC_MODE(d) = STUDY_SELECT_ALC_DISCOVERY;
}

static void select_paladin_mercies(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);

  int i = 0;
  int mercies_known = (CLASS_LEVEL(d->character, CLASS_PALADIN) / 3) - num_paladin_mercies_known(d->character);
  for (i = 0; i < NUM_PALADIN_MERCIES; i++)
    if (LEVELUP(d->character)->paladin_mercies[i])
      mercies_known--;

  write_to_output(d,
                  "\r\n-- %sSelect Paladin Mercies%s\r\n"
                  "\r\n",
                  mgn, nrm);

  for (i = 1; i < NUM_PALADIN_MERCIES; i++)
  {
    if ((i < 1) ||
        (i >= NUM_PALADIN_MERCIES) ||
        (KNOWS_MERCY(d->character, i)) ||
        (LEVELUP(d->character)->paladin_mercies[i]) ||
        (!can_learn_paladin_mercy(d->character, i)))
      continue;
    write_to_output(d, "%s %2d%s) %s : %s\r\n",
                    grn, i, nrm, paladin_mercies[i], paladin_mercy_descriptions[i]);
  }

  write_to_output(d, "\r\n"
                     "%s -1%s) Quit\r\n"
                     "\r\n"
                     "%d mercies can be selected\r\n"
                     "\r\n"
                     "If you wish to change the choices made in this screen, please quit the study session without saving.\r\n"
                     "\r\n"
                     "Enter Choice : ",
                  grn, nrm, mercies_known);

  OLC_MODE(d) = STUDY_SELECT_PAL_MERCY;
}

static void choose_necromancer_cast_type(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sSet Preferred Necromancer Casting Type%s\r\n"
                  "\r\n"
                  "%s 1%s) Arcane:%s\r\n"
                  "%s 2%s) Divine:%s\r\n"
                  "\r\n"
                  "%s-1%s) Quit\r\n"
                  "\r\n"
                  "Enter Choice : ",
                  mgn, nrm,
                  /* empty line */
                  grn, nrm, nrm,
                  grn, nrm, nrm,
                  /* empty line */
                  grn, nrm
                  /* empty line */
  );

  OLC_MODE(d) = STUDY_SET_NECROMANCER_CAST_TYPE;
}

static void choose_languages(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);

  struct char_data *ch = d->character;

  int i = 0, count = 0;
  int langs_known = num_languages_learned(ch);
  int langs_can_learn = MAX(0, GET_REAL_INT_BONUS(ch)) + MAX(0, GET_ABILITY(ch, ABILITY_LINGUISTICS)) + MAX(0, LEVELUP(ch)->skills[ABILITY_LINGUISTICS]);

  for (i = 0; i < NUM_LANGUAGES; i++)
  {
    if (LEVELUP(ch)->languages[i])
      langs_known++;
  }

  write_to_output(d,
                  "\r\n-- %sSelect Languages%s\r\n"
                  "\r\n",
                  mgn, nrm);

  for (i = 0; i < NUM_LANGUAGES; i++)
  {
    if (!CAN_SPEAK(ch, i))
    {
      if (LEVELUP(ch)->languages[i])
        send_to_char(ch, "\tM%-2d) %-35s \tn", i, languages[i]);
      else
        send_to_char(ch, "\tn%-2d) %-35s \tn", i, languages[i]);
      if ((count % 2) == 1)
      {
        send_to_char(ch, "\r\n");
      }
      count++;
    }
  }

  if ((count % 2) != 1)
  {
    send_to_char(ch, "\r\n");
  }

  write_to_output(d, "\r\n"
                     "%s -1%s) Quit\r\n"
                     "\r\n"
                     "%d language(s) can be selected\r\n"
                     "\r\n"
                     "Enter Choice : ",
                  grn, nrm, langs_can_learn - langs_known);

  OLC_MODE(d) = STUDY_CHOOSE_LANGUAGES;
}

static void select_blackguard_cruelties(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);

  int i = 0;
  int cruelties_known = (CLASS_LEVEL(d->character, CLASS_BLACKGUARD) / 3) - num_blackguard_cruelties_known(d->character);
  for (i = 0; i < NUM_BLACKGUARD_CRUELTIES; i++)
    if (LEVELUP(d->character)->blackguard_cruelties[i])
      cruelties_known--;

  write_to_output(d,
                  "\r\n-- %sSelect Blackguard Cruelties%s\r\n"
                  "\r\n",
                  mgn, nrm);

  for (i = 1; i < NUM_BLACKGUARD_CRUELTIES; i++)
  {
    if ((i < 1) ||
        (i >= NUM_BLACKGUARD_CRUELTIES) ||
        (KNOWS_CRUELTY(d->character, i)) ||
        (LEVELUP(d->character)->blackguard_cruelties[i]) ||
        (!can_learn_blackguard_cruelty(d->character, i)))
      continue;
    write_to_output(d, "%s %2d%s) %s : %s\r\n",
                    grn, i, nrm, blackguard_cruelties[i], blackguard_cruelty_descriptions[i]);
  }

  write_to_output(d, "\r\n"
                     "%s -1%s) Quit\r\n"
                     "\r\n"
                     "%d cruelties can be selected\r\n"
                     "\r\n"
                     "If you wish to change the choices made in this screen, please quit the study session without saving.\r\n"
                     "\r\n"
                     "Enter Choice : ",
                  grn, nrm, cruelties_known);

  OLC_MODE(d) = STUDY_SELECT_BG_CRUELTY;
}

static void select_high_elf_cantrip(struct descriptor_data *d)
{
  int i = 1;

  send_to_char(d->character, "Available Cantrips:\r\n");
  for (i = 1; i < NUM_SPELLS; i++)
  {
    if (i == SPELL_ENCHANT_ITEM)
      continue;
    if (spell_info[i].min_level[CLASS_WIZARD] == 1)
      send_to_char(d->character, "%3d) %s\r\n", i, spell_info[i].name);
  }
}

static void set_dragonborn_draconic_ancestry(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d, "\r\n");
  write_to_output(d, "Your draconic ancestry determines the damage type for your\r\n");
  write_to_output(d, "breath weapon and energy resistance once acquired.\r\n");
  write_to_output(d, "\r\n");
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_BLACK, DRCHRTLIST_NAME(DRACONIC_HERITAGE_BLACK), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_BLACK));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_BLUE, DRCHRTLIST_NAME(DRACONIC_HERITAGE_BLUE), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_BLUE));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_GREEN, DRCHRTLIST_NAME(DRACONIC_HERITAGE_GREEN), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_GREEN));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_RED, DRCHRTLIST_NAME(DRACONIC_HERITAGE_RED), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_RED));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_WHITE, DRCHRTLIST_NAME(DRACONIC_HERITAGE_WHITE), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_WHITE));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_BRASS, DRCHRTLIST_NAME(DRACONIC_HERITAGE_BRASS), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_BRASS));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_BRONZE, DRCHRTLIST_NAME(DRACONIC_HERITAGE_BRONZE), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_BRONZE));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_COPPER, DRCHRTLIST_NAME(DRACONIC_HERITAGE_COPPER), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_COPPER));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_SILVER, DRCHRTLIST_NAME(DRACONIC_HERITAGE_SILVER), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_SILVER));
  write_to_output(d, "%2d) %15s dragon - %s damage\r\n", DRACONIC_HERITAGE_GOLD, DRCHRTLIST_NAME(DRACONIC_HERITAGE_GOLD), DRCHRT_ENERGY_TYPE(DRACONIC_HERITAGE_GOLD));
  write_to_output(d, "\r\n");
  write_to_output(d, "\r\n%sSelect the dragon type for your draconic ancestry : ", nrm);
}

static void set_domain_submenu(struct descriptor_data *d)
{
  const char *domain_names[NUM_DOMAINS];
  int i;

  get_char_colors(d->character);
  clear_screen(d);

  for (i = 0; i < NUM_DOMAINS - 1; i++)
  {
    domain_names[i] = domain_list[i + 1].name;
  }

  column_list(d->character, 0, domain_names, NUM_DOMAINS - 1, TRUE);
  write_to_output(d, "\r\n%sEnter domain name selection : ", nrm);
}
static void set_domain_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sSet Domains%s\r\n"
                  "\r\n"
                  "%s 1%s) 1st Domain:      %s%s\r\n"
                  "%s 2%s) 2nd Domain:      %s%s\r\n"
                  "\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "Enter Choice : ",

                  mgn, nrm,
                  /* empty line */
                  grn, nrm, domain_list[GET_1ST_DOMAIN(d->character)].name, nrm,
                  grn, nrm, domain_list[GET_2ND_DOMAIN(d->character)].name, nrm,
                  /* empty line */
                  grn, nrm
                  /* empty line */
  );

  OLC_MODE(d) = STUDY_SET_DOMAINS;
}

void print_school_info(struct descriptor_data *d, int school_number)
{

  write_to_output(d, "\r\n");
  write_to_output(d, "%s\r\n", school_benefits[school_number]);
  write_to_output(d, "\r\n");
}

void print_domain_info(struct descriptor_data *d, int domain_number)
{
  int j = 0;

  write_to_output(d, "%sDomain:%s %-20s %sFavored Weapon:%s %-22s\r\n%sDescription:%s %s\r\n",
                  cyn, nrm, domain_list[domain_number].name,
                  cyn, nrm, weapon_list[domain_list[domain_number].favored_weapon].name,
                  cyn, nrm, domain_list[domain_number].description);

  write_to_output(d, "%sGranted powers: |%s", cyn, nrm);
  for (j = 0; j < MAX_GRANTED_POWERS; j++)
  {
    if (domain_list[domain_number].granted_powers[j] != DOMAIN_POWER_UNDEFINED)
    {
      write_to_output(d, "%s%s|%s", domainpower_names[domain_list[domain_number].granted_powers[j]], cyn, nrm);
    }
  }
  write_to_output(d, "\r\n");

  write_to_output(d, "%sGranted spells: |%s", cyn, nrm);
  for (j = 0; j < MAX_DOMAIN_SPELLS; j++)
  {
    if (domain_list[domain_number].domain_spells[j] != SPELL_RESERVED_DBC)
    {
      write_to_output(d, "%s%s|%s", spell_info[domain_list[domain_number].domain_spells[j]].name, cyn, nrm);
    }
  }
}

static void favored_enemy_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sRanger Favored Enemy Menu%s\r\n"
                  "\r\n"
                  "%s 0%s) Favored Enemy #1  (Min. Level  1):  %s%s\r\n"
                  "%s 1%s) Favored Enemy #2  (Min. Level  5):  %s%s\r\n"
                  "%s 2%s) Favored Enemy #3  (Min. Level 10):  %s%s\r\n"
                  "%s 3%s) Favored Enemy #4  (Min. Level 15):  %s%s\r\n"
                  "%s 4%s) Favored Enemy #5  (Min. Level 20):  %s%s\r\n"
                  "%s 5%s) Favored Enemy #6  (Min. Level 25):  %s%s\r\n"
                  "%s 6%s) Favored Enemy #7  (Min. Level 30):  %s%s\r\n"
                  "%s 7%s) Favored Enemy #8  (Min. Level xx):  %s%s\r\n"
                  "%s 8%s) Favored Enemy #9  (Min. Level xx):  %s%s\r\n"
                  "%s 9%s) Favored Enemy #10 (Min. Level xx):  %s%s\r\n"
                  "\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "Enter Choice : ",

                  mgn, nrm,
                  /* empty line */
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 0)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 1)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 2)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 3)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 4)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 5)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 6)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 7)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 8)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 9)], nrm,
                  /* empty line */
                  grn, nrm
                  /* empty line */
  );

  OLC_MODE(d) = FAVORED_ENEMY;
}

static void animal_companion_menu(struct descriptor_data *d)
{
  int i = 1, found = 0;

  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sDruid/Ranger Animal Companion Menu%s\r\n"
                  "\r\n",
                  mgn, nrm);

  for (i = 1; i <= TOP_OF_C; i++)
  {
    write_to_output(d, "%s\r\n", animal_names[i]);
  }

  write_to_output(d, "\r\n");
  /* find current animal */
  for (i = 1; i <= TOP_OF_C; i++)
  {
    if (GET_ANIMAL_COMPANION(d->character) == animal_vnums[i])
    {
      write_to_output(d, "Current Companion:  %s\r\n", animal_names[i]);
      found = 1;
      break;
    }
  }

  if (!found)
    write_to_output(d, "Currently No Companion Selected\r\n");

  write_to_output(d, "You can select 0 (Zero) to deselect the current "
                     "companion.\r\n");
  write_to_output(d, "\r\n"
                     "%s Q%s) Quit\r\n"
                     "\r\n"
                     "Enter Choice : ",
                  grn, nrm);

  OLC_MODE(d) = ANIMAL_COMPANION;
}

static void familiar_menu(struct descriptor_data *d)
{
  int i = 1, found = 0;

  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sFamiliar Menu%s\r\n"
                  "\r\n",
                  mgn, nrm);

  for (i = 1; i <= TOP_OF_C; i++)
  {
    write_to_output(d, "%s\r\n", familiar_names[i]);
  }

  write_to_output(d, "\r\n");

  /* find current familiar */
  for (i = 1; i <= TOP_OF_C; i++)
  {
    if (GET_FAMILIAR(d->character) == familiar_vnums[i])
    {
      write_to_output(d, "Current Familiar:  %s\r\n", familiar_names[i]);
      found = 1;
      break;
    }
  }

  if (!found)
    write_to_output(d, "Currently No Familiar Selected\r\n");

  write_to_output(d, "You can select 0 (Zero) to deselect the current "
                     "familiar.\r\n");
  write_to_output(d, "\r\n"
                     "%s Q%s) Quit\r\n"
                     "\r\n"
                     "Enter Choice : ",
                  grn, nrm);

  OLC_MODE(d) = FAMILIAR_MENU;
}

/* Helper function for the below menu */
bool can_study_feat_type(struct char_data *ch, int feat_type)
{
  int i = 0;
  bool result = FALSE;

  if (!CAN_STUDY_FEATS(ch))
    return FALSE;

  /*  Feat types divide the huge mess of feats into categories for the purpose
   *  of making the learning process easier. */

  for (i = 0; i < NUM_FEATS; i++)
  {
    if (feat_list[i].in_game &&
        (feat_list[i].feat_type == feat_type))
    {
      if (feat_is_available(ch, i, 0, NULL))
        result = TRUE;
    }
  }
  return result;
}

static void main_feat_disp_menu(struct descriptor_data *d)
{
  int i;
  bool can_study = FALSE;
  struct char_data *ch = d->character;

  get_char_colors(ch);
  clear_screen(d);

  LEVELUP(ch)->feat_type = -1;

  write_to_output(d,
                  "\r\n-- %sFeat Menu\r\n"
                  "\r\n",
                  mgn);
  for (i = 1; i < NUM_LEARNABLE_FEAT_TYPES; i++)
  {
    can_study = can_study_feat_type(ch, i);
    write_to_output(d,
                    "%s %d%s) %s%s\r\n",
                    (can_study ? grn : "\tD"), i, (can_study ? nrm : "\tD"), feat_types[i],
                    (can_study ? "" : "*"));
  }
  write_to_output(d,
                  "\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "* - An asterisk indicates you don't have access to this option.\r\n"
                  "Enter Choice : ",
                  grn, nrm);

  OLC_MODE(d) = STUDY_MAIN_FEAT_MENU;
}

static void main_boosts_disp_menu(struct descriptor_data *d)
{

  struct char_data *ch = d->character;

  get_char_colors(ch);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sAbility Score Boost Menu\r\n"
                  "\r\n",
                  grn);

  send_to_char(ch, "Boosts Remaining: %s%d%s\r\n\r\n"

                   "Strength     : %d %s--> %d%s\r\n"
                   "Dexterity    : %d %s--> %d%s\r\n"
                   "Constitution : %d %s--> %d%s\r\n"
                   "Intelligence : %d %s--> %d%s\r\n"
                   "Wisdom       : %d %s--> %d%s\r\n"
                   "Charisma     : %d %s--> %d%s\r\n",

               mgn, GET_LEVELUP_BOOSTS(ch), nrm,
               GET_REAL_STR(ch), GET_LEVELUP_BOOST_STATS(ch, 0) > 0 ? mgn : nrm, GET_REAL_STR(ch) + GET_LEVELUP_BOOST_STATS(ch, 0), nrm,
               GET_REAL_DEX(ch), GET_LEVELUP_BOOST_STATS(ch, 1) > 0 ? mgn : nrm, GET_REAL_DEX(ch) + GET_LEVELUP_BOOST_STATS(ch, 1), nrm,
               GET_REAL_CON(ch), GET_LEVELUP_BOOST_STATS(ch, 2) > 0 ? mgn : nrm, GET_REAL_CON(ch) + GET_LEVELUP_BOOST_STATS(ch, 2), nrm,
               GET_REAL_INT(ch), GET_LEVELUP_BOOST_STATS(ch, 3) > 0 ? mgn : nrm, GET_REAL_INT(ch) + GET_LEVELUP_BOOST_STATS(ch, 3), nrm,
               GET_REAL_WIS(ch), GET_LEVELUP_BOOST_STATS(ch, 4) > 0 ? mgn : nrm, GET_REAL_WIS(ch) + GET_LEVELUP_BOOST_STATS(ch, 4), nrm,
               GET_REAL_CHA(ch), GET_LEVELUP_BOOST_STATS(ch, 5) > 0 ? mgn : nrm, GET_REAL_CHA(ch) + GET_LEVELUP_BOOST_STATS(ch, 5), nrm

  );

  send_to_char(ch, "\r\n"
                   "Type the name of the ability score you wish to change.\r\n"
                   "Type it once to assign your boost to that score.\r\n"
                   "Type it again to undo that choice.\r\n"
                   "Type quit to exit this menu.\r\n"
                   "Your Choice: ");

  OLC_MODE(d) = STUDY_MAIN_BOOSTS_MENU;
}

static void main_skills_disp_menu(struct descriptor_data *d)
{
  int i, start_ability, end_ability;
  ;
  // bool can_study = FALSE;
  struct char_data *ch = d->character;

  start_ability = START_GENERAL_ABILITIES;
  end_ability = END_GENERAL_ABILITIES + 1;

  get_char_colors(ch);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sSkills Menu\r\n"
                  "\r\n",
                  mgn);

  send_to_char(ch, "*Name of skill, invested points, total points with all active bonuses\tn\r\n"
                   "\tcSkill              Inve Tota Class/Cross/Unavailable  \tMUnspent trains: \tm%d\tn\r\n",
               GET_LEVELUP_SKILL_POINTS(ch));

  for (i = start_ability; i < end_ability; i++)
  {
    /* we have some unused defines right now, we are going to skip over
       them manaully */
    switch (i)
    {
    case ABILITY_UNUSED_1:
    case ABILITY_UNUSED_2:
    case ABILITY_UNUSED_3:
    case ABILITY_UNUSED_4:
    case ABILITY_UNUSED_5:
    case ABILITY_UNUSED_6:
    case ABILITY_UNUSED_7:
      continue;
    default:
      break;
    }
    send_to_char(ch, "%-18s [%2d] \tC[%2d]\tn %s\r\n",
                 ability_names[i], GET_ABILITY(ch, i) + LEVELUP(ch)->skills[i], compute_ability(ch, i),
                 cross_names[modify_class_ability(ch, i, GET_CLASS(ch))]);
  }

  send_to_char(ch, "Please type quit or the name of the skill you wish to increase in rank: ");

  OLC_MODE(d) = STUDY_MAIN_SKILLS_MENU;
}

static void display_study_feats(struct descriptor_data *d)
{
  struct char_data *ch = d->character;
  int i = 0, j = 0, feat_marker = 0, feat_counter = 0, sortpos = 0;
  int count = 0;
  bool class_feat = FALSE;
  struct class_feat_assign *feat_assign = NULL;
  int class = LEVELUP(ch)->class;

  /* we're traversing the list of feats in alphabetical order */
  for (sortpos = 1; sortpos < NUM_FEATS; sortpos++)
  {
    /* i = the feat we are now handling */
    i = feat_sort_info[sortpos];
    feat_marker = 1;    /* using this to mark the class feat list */
    feat_counter = 0;   /* counter used for traversing class-feats list */
    class_feat = FALSE; /* is this a class feat? */

    /* we now traverse the class's list of class-feats to see if 'i' matches */
    for (feat_assign = class_list[class].featassign_list; feat_assign != NULL;
         feat_assign = feat_assign->next)
    {
      /* is this a class feat?  and is this feat a match for 'i'? */
      if (feat_assign->is_classfeat && feat_assign->feat_num == i)
      {
        class_feat = TRUE; /* yep this is a class feat! */
      }
      else if (is_class_feat(i, class, ch))
      {
        class_feat = TRUE;
      }
    }

    j = 0;

    if (((feat_list[i].feat_type == LEVELUP(ch)->feat_type) &&
         feat_is_available(ch, i, 0, NULL) &&
         feat_list[i].in_game &&
         feat_list[i].can_learn &&
         (!has_feat_requirement_check(ch, i) || feat_list[i].can_stack)))
    {

      write_to_output(d,
                      "%s%s%3d%s) %-30s%s",
                      (class_feat ? (feat_list[i].epic ? "\tM(EC)" : "\tC (C)") : (feat_list[i].epic ? "\tM (E)" : "    ")),
                      grn, i, nrm, feat_list[i].name, nrm);
      count++;

      if (count % 2 == 0)
        write_to_output(d, "\r\n");
    }
  }

  if (count % 2 != 0)
    write_to_output(d, "\r\n");

  write_to_output(d, "\r\n");

  write_to_output(d,
                  "To view more info about a feat, just select the number beside it.\r\n  "
                  "Class feats are in \tCcyan\tn and marked with a (C).\r\n"
                  "Epic feats, both class and regular, are in \tMMagenta\tn and are marked with (EC) or (E).\r\n");
  write_to_output(d, "Feat Points: General (%s%d%s) Class (%s%d%s) Epic (%s%d%s) Epic Class (%s%d%s) Teamwork (%s%d%s)\r\n",
                  (LEVELUP(ch)->feat_points > 0 ? grn : red), LEVELUP(ch)->feat_points, nrm,
                  (LEVELUP(ch)->class_feat_points > 0 ? grn : red), LEVELUP(ch)->class_feat_points, nrm,
                  (LEVELUP(ch)->epic_feat_points > 0 ? grn : red), LEVELUP(ch)->epic_feat_points, nrm,
                  (LEVELUP(ch)->epic_class_feat_points > 0 ? grn : red), LEVELUP(ch)->epic_class_feat_points, nrm,
                  (LEVELUP(ch)->teamwork_feat_points > 0 ? grn : red), LEVELUP(ch)->teamwork_feat_points, nrm);

  write_to_output(d, "Your choice? (type -1 or Q to exit) : ");
}

static void gen_feat_disp_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sFeats%s\r\n"
                  "\r\n",
                  mgn, nrm);

  display_study_feats(d);

  OLC_MODE(d) = STUDY_GEN_FEAT_MENU;
}

void show_dragon_rider_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d, "\r\n-- %sDragon Rider%s\r\n"
                     "\r\n",
                     mgn, nrm);
  write_to_output(d, "1) Dragon Mount Type: %s (%s)\r\n", 
    LEVELUP(d->character)->dragon_rider_dragon_type ? draconic_heritage_names[LEVELUP(d->character)->dragon_rider_dragon_type] : "unchosen", 
    LEVELUP(d->character)->dragon_rider_dragon_type ? damtypes[draconic_heritage_energy_types[LEVELUP(d->character)->dragon_rider_dragon_type]] : "unchosen");
  write_to_output(d, "2) Dragon Bond Type:  %s\r\n", 
    LEVELUP(d->character)->dragon_rider_bond_type ? dragon_bond_types[LEVELUP(d->character)->dragon_rider_bond_type] : "unchosen");
  write_to_output(d, "Q) Exit this menu.\r\n");
  write_to_output(d, "\r\n");
  OLC_MODE(d) = STUDY_DRAGON_RIDER_MENU;
}

void show_dragon_rider_bond_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d, "\r\n");
  write_to_output(d, "-- Select Dragon Rider Bond Type:\r\n");
  write_to_output(d, "\r\n");
  write_to_output(d, "1) Dragon Champion\r\n");
  write_to_output(d, "2) Dragon Scion\r\n");
  write_to_output(d, "3) Dragon Kin\r\n");
  write_to_output(d, "Q) Return to Main Dragon Rider Menu\r\n");
  write_to_output(d, "\r\n");
  OLC_MODE(d) = STUDY_DRAGON_RIDER_BOND_TYPE;
}

static void generic_main_disp_menu(struct descriptor_data *d)
{
  struct char_data *ch = d->character;
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sStudy Menu\r\n"
                  "\r\n"
                  "%s 0%s) Skills%s\r\n"
                  "%s 1%s) Ability Score Boosts%s\r\n"
                  "%s 2%s) Feats%s\r\n"
                  "%s 3%s) Known Spells%s\r\n"
                  "%s 4%s) Known Psionic Powers%s\r\n"
                  "%s 5%s) Choose Familiar%s\r\n"
                  "%s 6%s) Animal Companion%s\r\n"
                  "%s 7%s) Ranger Favored Enemy%s\r\n"
                  "%s 8%s) Cleric Domain Selection%s\r\n"
                  "%s 9%s) Wizard School Selection%s\r\n"
                  "%s A%s) Preferred Caster Classes (Prestige)%s\r\n"
                  "%s B%s) Sorcerer Bloodline Selection%s\r\n"
                  "%s C%s) Alchemist Discoveries Selection%s\r\n"
                  "%s D%s) Paladin Mercies%s\r\n"
                  "%s E%s) Blackguard Cruelties%s\r\n"
                  "%s F%s) Summoner Eidolons%s\r\n"
                  "%s G%s) Racial Abilities Selection%s\r\n"
                  "%s H%s) Necromancer Casting Type%s\r\n"
                  "%s I%s) Languages%s\r\n"
                  "%s J%s) Dragon Riders%s\r\n"
                  "\r\n"
                  "%s R%s) Reset Character%s\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "* - An asterisk indicates you don't have access to this option.  %s\r\n"
                  "Enter Choice : ",

                  mgn,
                  MENU_OPT(CAN_STUDY_SKILLS(ch)), (CAN_STUDY_SKILLS(ch)) ? "" : "*",                                   // 0
                  MENU_OPT(CAN_STUDY_BOOSTS(ch)), CAN_STUDY_BOOSTS(ch) ? "" : "*",                                     // 1
                  MENU_OPT(CAN_STUDY_FEATS(ch)), CAN_STUDY_FEATS(ch) ? "" : "*",                                       // 2
                  MENU_OPT(CAN_STUDY_KNOWN_SPELLS(ch)), CAN_STUDY_KNOWN_SPELLS(ch) ? "" : "*",                         // 3
                  MENU_OPT(CAN_STUDY_KNOWN_PSIONICS(ch)), CAN_STUDY_KNOWN_PSIONICS(ch) ? "" : "*",                     // 4
                  MENU_OPT(CAN_STUDY_FAMILIAR(ch)), CAN_STUDY_FAMILIAR(ch) ? "" : "*",                                 // 5
                  MENU_OPT(CAN_STUDY_COMPANION(ch)), CAN_STUDY_COMPANION(ch) ? "" : "*",                               // 6
                  MENU_OPT(CAN_STUDY_FAVORED_ENEMY(ch)), CAN_STUDY_FAVORED_ENEMY(ch) ? "" : "*",                       // 7
                  MENU_OPT(CAN_SET_DOMAIN(ch)), CAN_SET_DOMAIN(ch) ? "" : "*",                                         // 8
                  MENU_OPT(CAN_SET_SCHOOL(ch)), CAN_SET_SCHOOL(ch) ? "" : "*",                                         // 9
                  MENU_OPT(CAN_SET_P_CASTER(ch)), CAN_SET_P_CASTER(ch) ? "" : "*",                                     // A
                  MENU_OPT(CAN_SET_S_BLOODLINE(ch)), CAN_SET_S_BLOODLINE(ch) ? "" : "*",                               // B
                  MENU_OPT(has_alchemist_discoveries_unchosen(ch)), has_alchemist_discoveries_unchosen(ch) ? "" : "*", // C
                  MENU_OPT(has_paladin_mercies_unchosen(ch)), has_paladin_mercies_unchosen(ch) ? "" : "*",             // D
                  MENU_OPT(has_blackguard_cruelties_unchosen(ch)), has_blackguard_cruelties_unchosen(ch) ? "" : "*",   // E
                  MENU_OPT(has_eidolon_choices_unchosen(ch)), has_eidolon_choices_unchosen(ch) ? "" : "*",             // F
                  MENU_OPT(has_racial_abils_unchosen(ch)), has_racial_abils_unchosen(ch) ? "" : "*",                   // G
                  MENU_OPT(has_necromancer_cast_type_unchosen(ch)), has_necromancer_cast_type_unchosen(ch) ? "" : "*",  // H
                  MENU_OPT(has_unchosen_languages(ch)), has_unchosen_languages(ch) ? "" : "*",                         // I
                  MENU_OPT(has_unchosen_dragon_rider(ch)), has_unchosen_dragon_rider(ch) ? "" : "*",                   // J
                  MENU_OPT(GET_LEVEL(ch) == 1), GET_LEVEL(ch) == 1 ? "" : "*",                                         // R
                  grn, nrm,
                  (GET_PREMADE_BUILD_CLASS(ch) != CLASS_UNDEFINED) ? "(You are using premade build, options are limited!)" : "");

  OLC_MODE(d) = STUDY_GEN_MAIN_MENU;
}

/*  This does not work for all cfeats -exotic weapon proficiency is a special
 *  case and needs special handling. */

static void cfeat_disp_menu(struct descriptor_data *d)
{
  const char *feat_weapons[NUM_WEAPON_FAMILIES + 1];
  int i = 0, counter = 0;

  get_char_colors(d->character);
  clear_screen(d);

  /* we want to use column_list here, but we don't have a pre made list
   * of string values (without undefined).  Make one, and make sure it is in order. */
  for (i = 0; i < NUM_WEAPON_FAMILIES; i++)
  {
    feat_weapons[counter] = weapon_family[i];
    counter++;
  }

  column_list(d->character, 3, feat_weapons, counter, TRUE);

  write_to_output(d, "\r\n%sChoose weapon type for the %s feat : ", nrm, feat_list[LEVELUP(d->character)->tempFeat].name);

  OLC_MODE(d) = STUDY_CFEAT_MENU;
}

//#define USINGOLDFEATMENU

#ifdef USINGOLDFEATMENU

static void cfeat_disp_menu_old(struct descriptor_data *d)
{
  const char *feat_weapons[NUM_WEAPON_TYPES - 1];
  int i = 0, counter = 0;

  get_char_colors(d->character);
  clear_screen(d);

  /* we want to use column_list here, but we don't have a pre made list
   * of string values (without undefined).  Make one, and make sure it is in order. */
  for (i = 0; i < NUM_WEAPON_TYPES - 1; i++)
  {
    switch (i + 1)
    { /* this switch necessary due to composite bow strengths */
    case WEAPON_TYPE_COMPOSITE_LONGBOW_2:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_3:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_4:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_5:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_2:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_3:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_4:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_5:
      break; /* do nothing! we don't want this listed */
    default: /* normal cases! */
      feat_weapons[counter] = weapon_list[i + 1].name;
      counter++;
      break;
    }
  }

  column_list(d->character, 3, feat_weapons, counter, TRUE);

  write_to_output(d, "\r\n%sChoose weapon type for the %s feat : ", nrm, feat_list[LEVELUP(d->character)->tempFeat].name);

  OLC_MODE(d) = STUDY_CFEAT_MENU;
}

#endif

static void sfeat_disp_menu(struct descriptor_data *d)
{
  int i = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (i = 1; i < NUM_SCHOOLS; i++)
    write_to_output(d, "%d) %s\r\n", i, spell_schools[i]);

  write_to_output(d, "\r\n%sChoose a spell school for the %s feat : ", nrm, feat_list[LEVELUP(d->character)->tempFeat].name);

  OLC_MODE(d) = STUDY_SFEAT_MENU;
}

static void skfeat_disp_menu(struct descriptor_data *d)
{
  int i = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (i = 1; i < END_GENERAL_ABILITIES + 1; i++)
  {
    switch (i)
    {
    case ABILITY_UNUSED_1:
    case ABILITY_UNUSED_2:
    case ABILITY_UNUSED_3:
    case ABILITY_UNUSED_4:
    case ABILITY_UNUSED_5:
    case ABILITY_UNUSED_6:
    case ABILITY_UNUSED_7:
      continue;
    }
    write_to_output(d, "%d) %s\r\n", i, ability_names[i]);
  }

  write_to_output(d, "\r\n%sChoose a skill for the %s feat : ", nrm,
                  feat_list[LEVELUP(d->character)->tempFeat].name);

  OLC_MODE(d) = STUDY_SKFEAT_MENU;
}

/**************************************************************************
  The handler
 **************************************************************************/

/* we add to add this function to make sure training points are properly
 calculated based on stat changes */
void reset_training_points(struct char_data *ch)
{
  int i = 0, trains = 0, int_bonus = 0;

  /* first reset all trained abilities */
  for (i = 1; i <= NUM_ABILITIES; i++)
    SET_ABILITY(ch, i, 0);

  /* determine intelligence bonus */
  int_bonus = (int)((LEVELUP(ch)->inte - 10) / 2);

  /* use class_list to establish base skill points, 4 minimum */
  trains = MAX(4, ((CLSLIST_TRAINS(GET_CLASS(ch)) + int_bonus) * 4));

  /* human bonus */
  if (GET_RACE(ch) == RACE_HUMAN || GET_RACE(ch) == DL_RACE_HUMAN)
    trains += 4;
  if (GET_RACE(ch) == DL_RACE_HALF_ELF)
    trains += 2;

  /* finalize */
  LEVELUP(ch)->trains = trains;
}

void study_parse(struct descriptor_data *d, char *arg)
{
  struct char_data *ch = d->character;
  int number = -1;
  int counter;
  int points_left = 0, cost_for_number = 0, new_stat = 0;
  int intel_bonus = 0;
  int tempXP = 0;
  int i = 0;
  bool can_add_spell = TRUE, found = FALSE;
  char arg1[200] = {'\0'}, arg2[200] = {'\0'};
  char buf[200] = {'\0'};

  two_arguments(arg, arg1, sizeof(arg1), arg2, sizeof(arg2));

  sprintf(arg, "%s", arg1);

  switch (OLC_MODE(d))
  {
  case STUDY_CONFIRM_SAVE:
    switch (*arg)
    {
    case 'y':
    case 'Y':
      /* Save the temporary values in LEVELUP(d->character) to the
       * character, print a message, free the structures and exit. */
      write_to_output(d, "Your choices have been finalized!\r\n\r\n");
      // if (GET_LEVEL(ch) == 1) /* in case they modify their intelligence */
      //   reset_training_points(ch);
      finalize_study(d);
      save_char(d->character, 0);
      cleanup_olc(d, CLEANUP_ALL);
      free(LEVELUP(d->character));
      LEVELUP(d->character) = NULL;
      break;
    case 'n':
    case 'N':
      /* Discard the changes and exit. */
      write_to_output(d, "Your choices were NOT finalized.\r\n\r\n");
      cleanup_olc(d, CLEANUP_ALL);
      free(LEVELUP(d->character));
      LEVELUP(d->character) = NULL;
      break;
    default:
      write_to_output(d, "Invalid choice!\r\nDo you wish to save your changes ? : ");
      break;
    }
    return;

  case STUDY_GEN_MAIN_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      if (GET_LEVEL(ch) == 1)
      {
        write_to_output(d, "Your training points will be reset upon exit to "
                           "account for any changes made to your intelligence stat. (This will only occur at "
                           "level 1)\r\n");
      }
      write_to_output(d, "You can currently study as much as you want per level.\r\n");
      /* in the future we will probably change study to be limited to 1/ level ?*/
      write_to_output(d, "Do you wish to save your changes? : ");
      OLC_MODE(d) = STUDY_CONFIRM_SAVE;
      break;
    case '0':
      main_skills_disp_menu(d);
      break;
    case '1':
      if (!CAN_STUDY_BOOSTS(ch))
      {
        generic_main_disp_menu(d);
        write_to_output(d, "That is an invalid choice!\r\n");
        break;
      }
      main_boosts_disp_menu(d);
      break;
    case '2':
      main_feat_disp_menu(d);
      break;
    case '3':
      if (CAN_STUDY_KNOWN_SPELLS(ch))
      {
        if (LEVELUP(ch)->class == CLASS_SORCERER ||
            ((LEVELUP(ch)->class == CLASS_ARCANE_ARCHER || LEVELUP(ch)->class == CLASS_MYSTIC_THEURGE ||
              LEVELUP(ch)->class == CLASS_ARCANE_SHADOW || LEVELUP(ch)->class == CLASS_SPELLSWORD ||
              LEVELUP(ch)->class == CLASS_KNIGHT_OF_THE_THORN ||
              LEVELUP(ch)->class == CLASS_ELDRITCH_KNIGHT || (LEVELUP(ch)->class == CLASS_NECROMANCER && NECROMANCER_CAST_TYPE(ch) == 1)) &&
             GET_PREFERRED_ARCANE(ch) == CLASS_SORCERER))
          sorc_known_spells_disp_menu(d);
        else if (LEVELUP(ch)->class == CLASS_BARD ||
                 ((LEVELUP(ch)->class == CLASS_ARCANE_ARCHER || LEVELUP(ch)->class == CLASS_MYSTIC_THEURGE ||
                   LEVELUP(ch)->class == CLASS_ARCANE_SHADOW || LEVELUP(ch)->class == CLASS_SPELLSWORD ||
                   LEVELUP(ch)->class == CLASS_KNIGHT_OF_THE_THORN ||
                   LEVELUP(ch)->class == CLASS_ELDRITCH_KNIGHT || (LEVELUP(ch)->class == CLASS_NECROMANCER && NECROMANCER_CAST_TYPE(ch) == 1)) &&
                  GET_PREFERRED_ARCANE(ch) == CLASS_BARD))
          bard_known_spells_disp_menu(d);
        else if (LEVELUP(ch)->class == CLASS_SUMMONER ||
                 ((LEVELUP(ch)->class == CLASS_ARCANE_ARCHER || LEVELUP(ch)->class == CLASS_MYSTIC_THEURGE ||
                   LEVELUP(ch)->class == CLASS_ARCANE_SHADOW || LEVELUP(ch)->class == CLASS_SPELLSWORD ||
                   LEVELUP(ch)->class == CLASS_KNIGHT_OF_THE_THORN ||
                   LEVELUP(ch)->class == CLASS_ELDRITCH_KNIGHT || (LEVELUP(ch)->class == CLASS_NECROMANCER && NECROMANCER_CAST_TYPE(ch) == 1)) &&
                  GET_PREFERRED_ARCANE(ch) == CLASS_SUMMONER))
          summoner_known_spells_disp_menu(d);
        else if (LEVELUP(ch)->class == CLASS_INQUISITOR ||
                 ((LEVELUP(ch)->class == CLASS_MYSTIC_THEURGE || LEVELUP(ch)->class == CLASS_KNIGHT_OF_THE_SWORD || LEVELUP(ch)->class == CLASS_KNIGHT_OF_THE_ROSE || 
                 LEVELUP(ch)->class == CLASS_KNIGHT_OF_THE_SKULL || 
                 (LEVELUP(ch)->class == CLASS_NECROMANCER && NECROMANCER_CAST_TYPE(ch) == 2)) && 
                 GET_PREFERRED_DIVINE(ch) == CLASS_INQUISITOR))
          inquisitor_known_spells_disp_menu(d);
        else if (LEVELUP(ch)->class == CLASS_WARLOCK)
          warlock_known_spells_disp_menu(d);
      }
      else
      {
        write_to_output(d, "That is an invalid choice!\r\n");
        generic_main_disp_menu(d);
      }
      break;
    case '4':
      if (LEVELUP(ch) && LEVELUP(ch)->inte != GET_REAL_INT(ch))
      {
        send_to_char(ch, "\tYSince you have changed your intelligence ability score, you need to save and\r\nquit from study before you can select your powers.\r\n");
        break;
      }
      if (CAN_STUDY_KNOWN_PSIONICS(ch))
      {
        psionicist_known_powers_disp_menu(d);
      }
      else
      {
        write_to_output(d, "That is an invalid choice!\r\n");
        generic_main_disp_menu(d);
      }
      break;
    case '5':
      if (CAN_STUDY_FAMILIAR(ch))
        familiar_menu(d);
      else
      {
        write_to_output(d, "That is an invalid choice!\r\n");
        generic_main_disp_menu(d);
      }
      break;
    case '6':
      if (CAN_STUDY_COMPANION(ch))
        animal_companion_menu(d);
      else
      {
        write_to_output(d, "That is an invalid choice!\r\n");
        generic_main_disp_menu(d);
      }
      break;
    case '7':
      if (CAN_STUDY_FAVORED_ENEMY(ch))
        favored_enemy_menu(d);
      else
      {
        write_to_output(d, "That is an invalid choice!\r\n");
        generic_main_disp_menu(d);
      }
      break;
    /*
        case '7':
          if (CAN_SET_STATS(ch)) {
            set_stats_menu(d);
          } else {
            write_to_output(d, "That is an invalid choice!\r\n");
            generic_main_disp_menu(d);
          }
          break;
        */
    case '8':
      if (CAN_SET_DOMAIN(ch))
        set_domain_menu(d);
      else
      {
        write_to_output(d, "That is an invalid choice!\r\n");
        generic_main_disp_menu(d);
      }
      break;
    case '9':
      if (CAN_SET_SCHOOL(ch))
        set_school_menu(d);
      else
      {
        write_to_output(d, "That is an invalid choice!\r\n");
        generic_main_disp_menu(d);
      }
      break;
    case 'a':
    case 'A':
      if (CAN_SET_P_CASTER(ch))
        set_preferred_caster(d);
      else
      {
        write_to_output(d, "That is an invalid choice!\r\n");
        generic_main_disp_menu(d);
      }
      break;
    case 'b':
    case 'B':
      if (CAN_STUDY_FEATS(ch) && GET_LEVEL(ch) < LVL_IMMORT)
      {
        write_to_output(d, "Please choose your feat(s) first.\r\n");
      }
      else if (CAN_SET_S_BLOODLINE(ch))
        set_sorcerer_bloodline(d);
      else
      {
        write_to_output(d, "That is an invalid choice!\r\n");
        generic_main_disp_menu(d);
      }
      break;
    case 'c':
    case 'C':
      if (CAN_STUDY_FEATS(ch) && GET_LEVEL(ch) < LVL_IMMORT)
      {
        write_to_output(d, "Please choose your feat(s) first.\r\n");
      }
      else if (has_alchemist_discoveries_unchosen(ch))
      {
        select_alchemist_discoveries(d);
      }
      else
      {
        write_to_output(d, "That is an invalid choice!\r\n");
        generic_main_disp_menu(d);
      }
      break;

    case 'd':
    case 'D':
      if (CAN_STUDY_FEATS(ch) && GET_LEVEL(ch) < LVL_IMMORT)
      {
        write_to_output(d, "Please choose your feat(s) first.\r\n");
      }
      else if (has_paladin_mercies_unchosen(ch))
      {
        select_paladin_mercies(d);
      }
      else
      {
        write_to_output(d, "That is an invalid choice!\r\n");
        generic_main_disp_menu(d);
      }
      break;

    case 'e':
    case 'E':
      if (CAN_STUDY_FEATS(ch) && GET_LEVEL(ch) < LVL_IMMORT)
      {
        write_to_output(d, "Please choose your feat(s) first.\r\n");
      }
      else if (has_blackguard_cruelties_unchosen(ch))
      {
        select_blackguard_cruelties(d);
      }
      else
      {
        write_to_output(d, "That is an invalid choice!\r\n");
        generic_main_disp_menu(d);
      }
      break;

    case 'F':
    case 'f':
      study_eidolon_main_menu_select(d);
      break;

    case 'G':
    case 'g':
      if (CAN_STUDY_FEATS(ch) && GET_LEVEL(ch) < LVL_IMMORT)
      {
        write_to_output(d, "Please choose your feat(s) first.\r\n");
      }
      else if (has_racial_abils_unchosen(ch))
        select_racial_abils(d);
      else
      {
        write_to_output(d, "That is an invalid choice!\r\n");
        generic_main_disp_menu(d);
      }
      break;

    case 'h':
    case 'H':
      if (has_necromancer_cast_type_unchosen(ch))
      {
        choose_necromancer_cast_type(d);
      }
      else
      {
        write_to_output(d, "That is an invalid choice!\r\n");
        generic_main_disp_menu(d);
      }
      break;


    case 'i':
    case 'I':
      if (has_unchosen_languages(ch))
      {
        choose_languages(d);
      }
      else
      {
        write_to_output(d, "That is an invalid choice!\r\n");
        generic_main_disp_menu(d);
      }
      break;

    case 'j':
    case 'J':
      if (has_unchosen_dragon_rider(ch))
      {
        show_dragon_rider_menu(d);
      }
      else
      {
        write_to_output(d, "That is an invalid choice!\r\n");
        generic_main_disp_menu(d);
      }
      break;

    // reset levelup, level 1 only.
    case 'R':
    case 'r':
      if (GET_LEVEL(ch) != 1)
      {
        send_to_char(ch, "You can't reset your level after you've passed level 1.  Please exit the study menu and use the respec command instead.\r\n");
        break;
      }
      send_to_char(ch, "Are you sure you wish to reset your level?  This will undo all of your changes and make you a freshly made level one character again.\r\nY or N?");
      OLC_MODE(d) = STUDY_CONFIRM_RESET;
      break;
    default:
      write_to_output(d, "That is an invalid choice!\r\n");
      display_main_menu(d);
      break;
    }
    break;
  case STUDY_CONFIRM_RESET:
    switch (*arg)
    {
    case 'y':
    case 'Y':
      tempXP = GET_EXP(ch);
      /* Make sure that players can't make wildshaped forms permanent.*/
      SUBRACE(ch) = 0;
      IS_MORPHED(ch) = 0;
      if (affected_by_spell(ch, SKILL_WILDSHAPE))
      {
        affect_from_char(ch, SKILL_WILDSHAPE);
        send_to_char(ch, "You return to your normal form..\r\n");
      }
      do_start(ch);
      HAS_SET_STATS_STUDY(ch) = FALSE;
      GET_EXP(ch) = tempXP;
      send_to_char(ch, "You have reset your  character and can begin choosing stats, skills and feats anew.\r\n");
      save_char(d->character, 0);
      cleanup_olc(d, CLEANUP_ALL);
      free(LEVELUP(d->character));
      LEVELUP(d->character) = NULL;
      return;
    default:
      display_main_menu(d);
      break;
    }
    break;
  case STUDY_MAIN_FEAT_MENU:
    /* This is the menu where the player chooses feats - This menu is actually a
     * 'master menu' that drives the process. */
    switch (*arg)
    {
    case 'q':
    case 'Q':
      display_main_menu(d);
      break;
    default: /* Choose Feats */
      number = atoi(arg);
      if (!CAN_STUDY_FEATS(ch) || (number < 1) || (number >= NUM_LEARNABLE_FEAT_TYPES) || !can_study_feat_type(ch, number))
      {
        write_to_output(d, "That is an invalid choice!\r\n");
        main_feat_disp_menu(d);
        break;
      }
      LEVELUP(d->character)->feat_type = number;
      gen_feat_disp_menu(d);
      break;
    }
    break;

  case STUDY_MAIN_BOOSTS_MENU:

    if (!*arg)
    {
      send_to_char(d->character, "\r\nPlease type the full name of the ability score you wish to increase or type quit to leave this menu.\r\n");
      send_to_char(d->character, "Your choice (quit or ability score name) : ");
      return;
    }

    for (i = 0; i < strlen(arg); i++)
      arg[i] = tolower(arg[i]);

    if (is_abbrev(arg, "quit"))
    {
      // let's update trains
      if (GET_LEVELUP_BOOST_STATS(ch, 3) > 0)
        intel_bonus += ((GET_REAL_INT(ch) + 1) % 2) ? 0 : 1;
      GET_LEVELUP_SKILL_POINTS(ch) += intel_bonus;
      display_main_menu(d);
      return;
    }

    if (is_abbrev(arg, "strength"))
    {
      use_boost_point(ch, 0);
      break;
    }
    else if (is_abbrev(arg, "dexterity"))
    {
      use_boost_point(ch, 1);
      break;
    }
    else if (is_abbrev(arg, "constitution"))
    {
      use_boost_point(ch, 2);
      break;
    }
    else if (is_abbrev(arg, "intelligence"))
    {
      use_boost_point(ch, 3);
      break;
    }
    else if (is_abbrev(arg, "wisdom"))
    {
      use_boost_point(ch, 4);
      break;
    }
    else if (is_abbrev(arg, "charisma"))
    {
      use_boost_point(ch, 5);
      break;
    }
    else
    {
      send_to_char(d->character, "\r\nPlease type the full name of the ability score you wish to increase or type quit to leave this menu.\r\n");
      send_to_char(d->character, "Your choice (quit or ability score name) : ");
      return;
    }

    break;

  case STUDY_MAIN_SKILLS_MENU:

    if (!*arg)
    {
      send_to_char(d->character, "\r\nPlease type the full name of the skill you wish to increase or type quit to leave this menu.\r\n");
      send_to_char(d->character, "Your choice (quit or skill to improve) : ");
      return;
    }

    if (is_abbrev(arg, "quit"))
    {
      display_main_menu(d);
      return;
    }

    if (GET_LEVELUP_SKILL_POINTS(ch) <= 0)
    {
      send_to_char(d->character, "You do not have any skill points left to spend.\r\n");
      send_to_char(d->character, "Your choice (quit or skill to improve) : ");
      return;
    }

    int skill_num = find_ability_num(arg);

    if (skill_num < 1)
    {
      send_to_char(ch, "You do not know of that skill.\r\n");
      send_to_char(d->character, "Your choice (quit or skill to improve) : ");
      return;
    }

    // skill not available to this class
    if (modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 0)
    {
      send_to_char(ch, "This skill is not available to your class...\r\n");
      send_to_char(d->character, "Your choice (quit or skill to improve) : ");
      return;
    }

    // cross-class skill
    if (GET_LEVELUP_SKILL_POINTS(ch) < 2 && modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 1)
    {
      send_to_char(ch, "(Cross-Class) You don't have enough skill points to train that skill...\r\n");
      send_to_char(d->character, "Your choice (quit or skill to improve) : ");
      return;
    }
    if (GET_LEVELUP_ABILITY(ch, skill_num) >= ((int)((GET_LEVEL(ch) + 3) / 2)) && modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 1)
    {
      send_to_char(ch, "You are already fully trained for your level in that area.\r\n");
      send_to_char(d->character, "Your choice (quit or skill to improve) : ");
      return;
    }

    // class skill
    if (GET_LEVELUP_ABILITY(ch, skill_num) >= (GET_LEVEL(ch) + 3) && modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 2)
    {
      send_to_char(ch, "You are already fully trained for your level in that area.\r\n");
      send_to_char(d->character, "Your choice (quit or skill to improve) : ");
      return;
    }

    send_to_char(ch, "You train for a while...\r\n");
    GET_LEVELUP_SKILL_POINTS(ch)
    --;
    if (modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 1)
    {
      GET_LEVELUP_SKILL_POINTS(ch)
      --;
      send_to_char(ch, "You used two skill points to train a cross-class skill...\r\n");
    }
    GET_LEVELUP_SKILL(ch, skill_num)
    ++;

    if (GET_LEVELUP_ABILITY(ch, skill_num) >= (GET_LEVEL(ch) + 3))
      send_to_char(ch, "You are now fully trained for your level in that area.\r\n");
    if (skill_num == ABILITY_STEALTH && HAS_REAL_FEAT(ch, FEAT_PRACTICED_SNEAK))
      ;
    else if (GET_LEVELUP_ABILITY(ch, skill_num) >= ((int)((GET_LEVEL(ch) + 3) / 2)) && CLSLIST_ABIL(GET_CLASS(ch), skill_num) == 1)
      send_to_char(ch, "You are already fully trained for your level in that area.\r\n");

    main_skills_disp_menu(d);

    break;

  case STUDY_SELECT_ALC_DISCOVERY:
    number = atoi(arg);
    if (number == -1 || *arg == 'q' || *arg == 'Q')
    {
      display_main_menu(d);
      break;
    }

    /* Check if the discovery is available. */
    if ((number < 1) ||
        (number >= NUM_ALC_DISCOVERIES) ||
        (KNOWS_DISCOVERY(d->character, number)) ||
        (LEVELUP(d->character)->discoveries[number] == TRUE) ||
        (!can_learn_discovery(d->character, number)))
    {
      write_to_output(d, "Invalid discovery, try again.\r\n");
      break;
    }

    if (!has_alchemist_discoveries_unchosen_study(ch))
    {
      send_to_char(ch, "You cannot choose new discoveries at this time.  If you wish to change your choices in this study session, quit the study menu without saving the changes.\r\n");
      break;
    }

    /* Store the feat number in the work area in the data structure. */
    LEVELUP(d->character)->tempDiscovery = number;

    /* Display the description of the feat, and give the player an option. */
    write_to_output(d, "%s%s%s: %s\r\n\r\n"
                       "Choose this discovery? (y/n) : ",
                    nrm, alchemical_discovery_names[number], nrm, alchemical_discovery_descriptions[number]);

    OLC_MODE(d) = STUDY_CONFIRM_ADD_DISCOVERY;
    break;

  case STUDY_SELECT_PAL_MERCY:
    number = atoi(arg);
    if (number == -1 || *arg == 'q' || *arg == 'Q')
    {
      display_main_menu(d);
      break;
    }

    /* Check if the discovery is available. */
    if ((number < 1) ||
        (number >= NUM_PALADIN_MERCIES) ||
        (KNOWS_MERCY(d->character, number)) ||
        (LEVELUP(d->character)->paladin_mercies[number] == TRUE) ||
        (!can_learn_paladin_mercy(d->character, number)))
    {
      write_to_output(d, "Invalid mercy, try again.\r\n");
      break;
    }

    if (!has_paladin_mercies_unchosen_study(ch))
    {
      send_to_char(ch, "You cannot choose new mercies at this time.  "
                       "If you wish to change your choices in this study session, "
                       "quit the study menu without saving the changes.\r\n");
      break;
    }

    /* Store the mercy choice in the work area in the data structure. */
    LEVELUP(d->character)->tempMercy = number;

    /* Display the description of the feat, and give the player an option. */
    write_to_output(d, "%s%s%s: %s\r\n\r\n"
                       "Choose this mercy? (y/n) : ",
                    nrm, paladin_mercies[number], nrm, paladin_mercy_descriptions[number]);

    OLC_MODE(d) = STUDY_CONFIRM_ADD_MERCY;
    break;

  case STUDY_SET_NECROMANCER_CAST_TYPE:
    number = atoi(arg);
    if (number == -1 || *arg == 'q' || *arg == 'Q')
    {
      display_main_menu(d);
      break;
    }
    if (number <= 0 || number > 2)
    {
      write_to_output(d, "Please select either 1 for arcane spell progression or 2 for divine spell progression.\r\n");
      break;
    }
    LEVELUP(ch)->necromancer_bonus_levels = number;
    write_to_output(d, "Your necromancer levels will now count towards your %s spell progression.\r\n", (number == 1) ? "arcane" : "divine");
    display_main_menu(d);
    break;

  case STUDY_CHOOSE_LANGUAGES:
    number = atoi(arg);
    if (number == -1 || *arg == 'q' || *arg == 'Q')
    {
      display_main_menu(d);
      break;
    }

    /* Check if the language is available. */
    if ((number < LANG_COMMON) ||
        (number >= NUM_LANGUAGES) ||
        (CAN_SPEAK(d->character, number)))
    {
      write_to_output(d, "Invalid language, try again.\r\n");
      break;
    }

    if (LEVELUP(ch)->languages[number] == TRUE)
    {
      LEVELUP(ch)->languages[number] = FALSE;
      send_to_char(ch, "You de-select language '%s'\r\n", languages[number]);
    }
    else
    {
      int langs_known = num_languages_learned(ch);
      int langs_can_learn = MAX(0, GET_REAL_INT_BONUS(ch)) + MAX(0, GET_ABILITY(ch, ABILITY_LINGUISTICS)) + MAX(0, LEVELUP(ch)->skills[ABILITY_LINGUISTICS]);
      for (i = 0; i < NUM_LANGUAGES; i++)
      {
        if (LEVELUP(ch)->languages[i])
          langs_known++;
      }

      if ((langs_can_learn - langs_known) <= 0)
      {
        send_to_char(ch, "You cannot learn any more languages.\r\n");
      }
      else
      {
        LEVELUP(ch)->languages[number] = TRUE;
        send_to_char(ch, "You select language '%s'\r\n", languages[number]);
      }
    }

    choose_languages(d);
    break;

  case STUDY_SELECT_BG_CRUELTY:
    number = atoi(arg);
    if (number == -1 || *arg == 'q' || *arg == 'Q')
    {
      display_main_menu(d);
      break;
    }

    /* Check if the discovery is available. */
    if ((number < 1) ||
        (number >= NUM_BLACKGUARD_CRUELTIES) ||
        (KNOWS_CRUELTY(d->character, number)) ||
        (LEVELUP(d->character)->blackguard_cruelties[number] == TRUE) ||
        (!can_learn_blackguard_cruelty(d->character, number)))
    {
      write_to_output(d, "Invalid cruelty, try again.\r\n");
      break;
    }

    if (!has_blackguard_cruelties_unchosen_study(ch))
    {
      send_to_char(ch, "You cannot choose new cruelties at this time.  "
                       "If you wish to change your choices in this study session, "
                       "quit the study menu without saving the changes.\r\n");
      break;
    }

    /* Store the mercy choice in the work area in the data structure. */
    LEVELUP(d->character)->tempCruelty = number;

    /* Display the description of the feat, and give the player an option. */
    write_to_output(d, "%s%s%s: %s\r\n\r\n"
                       "Choose this cruelty? (y/n) : ",
                    nrm, blackguard_cruelties[number], nrm, blackguard_cruelty_descriptions[number]);

    OLC_MODE(d) = STUDY_CONFIRM_ADD_CRUELTY;
    break;

  case STUDY_CONFIRM_ADD_DISCOVERY:
    switch (*arg)
    {
    case 'n':
    case 'N':
      select_alchemist_discoveries(d);
      break;
    case 'y':
    case 'Y':
      LEVELUP(d->character)->discoveries[LEVELUP(ch)->tempDiscovery] = TRUE;
      write_to_output(d, "Alchemist Discovery %s chosen!\r\n", alchemical_discovery_names[LEVELUP(ch)->tempDiscovery]);
      LEVELUP(ch)->tempDiscovery = 0;
      select_alchemist_discoveries(d);
      break;
    }
    break;

  case STUDY_CONFIRM_ADD_MERCY:
    switch (*arg)
    {
    case 'n':
    case 'N':
      select_paladin_mercies(d);
      break;
    case 'y':
    case 'Y':
      LEVELUP(d->character)->paladin_mercies[LEVELUP(ch)->tempMercy] = TRUE;
      write_to_output(d, "Paladin Mercy %s chosen!\r\n", paladin_mercies[LEVELUP(ch)->tempMercy]);
      LEVELUP(ch)->tempMercy = 0;
      select_paladin_mercies(d);
      break;
    }
    break;

  case STUDY_CONFIRM_ADD_CRUELTY:
    switch (*arg)
    {
    case 'n':
    case 'N':
      select_blackguard_cruelties(d);
      break;
    case 'y':
    case 'Y':
      LEVELUP(d->character)->blackguard_cruelties[LEVELUP(ch)->tempCruelty] = TRUE;
      write_to_output(d, "Blackguard Cruelty %s chosen!\r\n", blackguard_cruelties[LEVELUP(ch)->tempCruelty]);
      LEVELUP(ch)->tempCruelty = 0;
      select_blackguard_cruelties(d);
      break;
    }
    break;

  case STUDY_GEN_FEAT_MENU:
    number = atoi(arg);
    if (number == -1 || *arg == 'q' || *arg == 'Q')
    {
      main_feat_disp_menu(d);
      break;
    }

    /* Check if the feat is available. */
    if ((number < 1) ||
        (number >= NUM_FEATS) ||
        (!feat_is_available(d->character, number, 0, NULL)) ||
        (!feat_list[number].can_learn) ||
        (has_feat_requirement_check(d->character, number) &&
         feat_list[number].can_stack == FALSE &&
         feat_list[number].combat_feat == FALSE))
    {
      write_to_output(d, "Invalid feat, try again.\r\n");
      gen_feat_disp_menu(d);
      break;
    }

    /* Store the feat number in the work area in the data structure. */
    LEVELUP(d->character)->tempFeat = number;

    /* Display the description of the feat, and give the player an option. */
    write_to_output(d, "%s%s%s: %s\r\n\r\n"
                       "Choose this feat? (y/n) : ",
                    nrm, feat_list[LEVELUP(ch)->tempFeat].name, nrm, feat_list[LEVELUP(ch)->tempFeat].description);

    OLC_MODE(d) = STUDY_CONFIRM_ADD_FEAT;
    break;

  case STUDY_CONFIRM_ADD_FEAT:
    switch (*arg)
    {
    case 'n':
    case 'N':
      gen_feat_disp_menu(d);
      break;
    case 'y':
    case 'Y':
      /* Check to see if this feat has a subfeat - If so, then display the
       * approptiate menus. */
      if (feat_to_cfeat(LEVELUP(ch)->tempFeat) != -1)
      {
        /* Combat feat - Need to choose weapon type. */
        cfeat_disp_menu(d);
        break;
      }
      if (feat_to_sfeat(LEVELUP(ch)->tempFeat) != -1)
      {
        /* Spell school feat - Need to choose spell school. */
        sfeat_disp_menu(d);
        break;
      }
      if (feat_to_skfeat(LEVELUP(ch)->tempFeat) != -1)
      {
        /* Skill feat - Need to choose a skill (ability) */
        skfeat_disp_menu(d);
        break;
      }

      if (add_levelup_feat(d, LEVELUP(ch)->tempFeat))
        write_to_output(d, "Feat %s chosen!\r\n", feat_list[LEVELUP(ch)->tempFeat].name);
      gen_feat_disp_menu(d);
      break;
    }
    break;
  /* Combat feats require the selection of a weapon type. */
  case STUDY_CFEAT_MENU:
    number = atoi(arg);
    if (number == -1 || *arg == 'q' || *arg == 'Q')
    {
      LEVELUP(d->character)->tempFeat = -1;
      gen_feat_disp_menu(d);
      break;
    }
    if ((number < 1) || (number > NUM_WEAPON_FAMILIES))
    {
      write_to_output(d, "That is an invalid choice!\r\n");
      cfeat_disp_menu(d);
      return;
    }
    number--; // menu starts at 1 but weapon family defines start at 0
    if (HAS_COMBAT_FEAT(ch, feat_to_cfeat(LEVELUP(d->character)->tempFeat), number) ||
        HAS_LEVELUP_COMBAT_FEAT(ch, feat_to_cfeat(LEVELUP(ch)->tempFeat), number))
    {
      write_to_output(d, "You already have that weapon type selected for this feat!\r\n\r\n");
      cfeat_disp_menu(d);
      return;
    }
    /* Now we have the weapon type - set it in the structure. */
    if (add_levelup_feat(d, LEVELUP(d->character)->tempFeat))
    {
      SET_LEVELUP_COMBAT_FEAT(d->character, feat_to_cfeat(LEVELUP(d->character)->tempFeat), number);

      write_to_output(d, "Feat %s (%s) chosen!\r\n",
                      feat_list[LEVELUP(d->character)->tempFeat].name,
                      weapon_family[number]);
    }
    else
    {
      LEVELUP(d->character)->tempFeat = -1;
    }
    gen_feat_disp_menu(d);

    break;

  /* School feats require the selection of a spell school. */
  case STUDY_SFEAT_MENU:
    number = atoi(arg);
    if (number == -1 || *arg == 'q' || *arg == 'Q')
    {
      LEVELUP(d->character)->tempFeat = -1;
      gen_feat_disp_menu(d);
      break;
    }
    if ((number < 1) || (number >= NUM_SCHOOLS))
    {
      write_to_output(d, "That is an invalid choice!\r\n");
      sfeat_disp_menu(d);
      break;
    }
    if (HAS_SCHOOL_FEAT(ch, feat_to_sfeat(LEVELUP(d->character)->tempFeat), number) ||
        HAS_LEVELUP_SCHOOL_FEAT(ch, feat_to_sfeat(LEVELUP(d->character)->tempFeat), number))
    {
      write_to_output(d, "You already have that school selected for this feat!\r\n\r\n");
      sfeat_disp_menu(d);
      break;
    }

    /* Now we have the spell school. */
    if (add_levelup_feat(d, LEVELUP(d->character)->tempFeat))
    {
      SET_LEVELUP_SCHOOL_FEAT(d->character, feat_to_sfeat(LEVELUP(d->character)->tempFeat), number);

      write_to_output(d, "Feat %s (%s) chosen!\r\n", feat_list[LEVELUP(d->character)->tempFeat].name, spell_schools[number]);
    }
    else
    {
      LEVELUP(d->character)->tempFeat = -1;
    }
    gen_feat_disp_menu(d);
    break;

  /* Skill feats require the selection of a skill. */
  case STUDY_SKFEAT_MENU:
    number = atoi(arg);
    if (number == -1 || *arg == 'q' || *arg == 'Q')
    {
      LEVELUP(d->character)->tempFeat = -1;
      gen_feat_disp_menu(d);
      break;
    }
    switch (number)
    {
    case ABILITY_UNUSED_1:
    case ABILITY_UNUSED_2:
    case ABILITY_UNUSED_3:
    case ABILITY_UNUSED_4:
    case ABILITY_UNUSED_5:
    case ABILITY_UNUSED_6:
    case ABILITY_UNUSED_7:
      write_to_output(d, "That is an invalid choice!\r\n");
      skfeat_disp_menu(d);
      break;
    }
    if ((number < 1) || (number > END_GENERAL_ABILITIES))
    {
      write_to_output(d, "That is an invalid choice!\r\n");
      skfeat_disp_menu(d);
      break;
    }
    if (HAS_SKILL_FEAT(ch, number, feat_to_skfeat(LEVELUP(d->character)->tempFeat)) ||
        HAS_LEVELUP_SKILL_FEAT(ch, number, feat_to_skfeat(LEVELUP(ch)->tempFeat)))
    {
      write_to_output(d, "You already have that skill selected for this feat!\r\n\r\n");
      skfeat_disp_menu(d);
      break;
    }
    /* Now we have the skill - set it in the structure. */
    if (add_levelup_feat(d, LEVELUP(d->character)->tempFeat))
    {
      SET_LEVELUP_SKILL_FEAT(d->character, number, feat_to_skfeat(LEVELUP(d->character)->tempFeat));

      write_to_output(d, "Feat %s (%s) chosen!\r\n", feat_list[LEVELUP(d->character)->tempFeat].name, ability_names[number]);
    }
    else
    {
      LEVELUP(d->character)->tempFeat = -1;
    }
    gen_feat_disp_menu(d);
    break;

  /******* start sorcerer **********/
  case STUDY_SORC_KNOWN_SPELLS_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      display_main_menu(d);
      break;
    case 'a':
    case 'A':
      if (free_arcana_slots(ch) <= 0)
      {
        write_to_output(d, "That is not a valid option.\r\n");
      }
      else
      {
        display_new_arcana_menu(d);
      }
      break;
      /* here are our spell levels for 'spells known' */
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      sorc_study_menu(d, atoi(arg));
      OLC_MODE(d) = STUDY_SPELLS;
      break;
    default:
      write_to_output(d, "That is an invalid choice!\r\n");
      sorc_known_spells_disp_menu(d);
      break;
    }
    break;

  case SET_NEW_ARCANA:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      sorc_known_spells_disp_menu(d);
      OLC_MODE(d) = STUDY_SORC_KNOWN_SPELLS_MENU;
      break;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      if (free_arcana_slots(d->character) <= 0)
      {
        write_to_output(d, "You do not have any free new arcana slots.\r\n");
      }
      else if (compute_slots_by_circle(d->character, CLASS_SORCERER, atoi(arg)) <= 0)
      {
        write_to_output(d, "You are not able to cast spells of that circle yet.\r\n");
      }
      else
      {
        write_to_output(d, "You've assigned an extra spell circle %s with your new arcana prowess.\r\n", arg);
        if (NEW_ARCANA_SLOT(d->character, 0) <= 0)
          NEW_ARCANA_SLOT(d->character, 0) = atoi(arg);
        else if (NEW_ARCANA_SLOT(d->character, 1) <= 0)
          NEW_ARCANA_SLOT(d->character, 1) = atoi(arg);
        else if (NEW_ARCANA_SLOT(d->character, 2) <= 0)
          NEW_ARCANA_SLOT(d->character, 2) = atoi(arg);
        write_to_output(d, "You have %d new arcana slots let to spend.\r\n", free_arcana_slots(d->character));
      }
      write_to_output(d, "Please select a spell circle, or type 'Q' to exit this menu.\r\nEnter your choice:");
      break;
    default:
      write_to_output(d, "That is an invalid choice!\r\n");
      write_to_output(d, "Please select a spell circle, or type 'Q' to exit this menu.\r\nEnter your choice:");
      break;
    }
    break;

  case STUDY_SPELLS:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      sorc_known_spells_disp_menu(d);
      break;

    default:
      number = atoi(arg);

      /* SPELL PREPARATION HOOK */
      for (counter = 1; counter < NUM_SPELLS; counter++)
      {
        if (counter == number)
        {
          if (compute_spells_circle(ch, CLASS_SORCERER,
                                    counter,
                                    METAMAGIC_NONE,
                                    DOMAIN_UNDEFINED) ==
              LEVELUP(d->character)->spell_circle)
          {
            if (*arg2 && is_abbrev(arg2, "help"))
            {
              snprintf(buf, sizeof(buf), "%s", spell_info[counter].name);
              do_help(d->character, buf, 0, 0);
              return;
            }
            if (is_a_known_spell(d->character, CLASS_SORCERER, counter))
            {
              if (!LEVELUP(d->character)->spells_learned[counter])
              {
                send_to_char(d->character, "\tCYou cannot remove spells known, unless it is a spell you already chose this level. "
                                           "To change past choices, you need "
                                           "to respec your character.\r\n\tn");
                break;
              }
              else
              {
                known_spells_remove_by_class(d->character, CLASS_SORCERER, counter);
                LEVELUP(d->character)->spells_learned[counter] = 0;
              }
            }
            else
            {
              can_add_spell = known_spells_add(d->character, CLASS_SORCERER, counter, FALSE);
              if (!can_add_spell)
              {
                write_to_output(d, "You are all FULL for spells!\r\n");
                break;
              }
              else
              {
                LEVELUP(d->character)->spells_learned[counter] = 1;
              }
            }
          }
        }
      }
      sorc_study_menu(d, LEVELUP(d->character)->spell_circle);
      break;
    }
    break;
    /******* end sorcerer **********/

    /******* start warlock **********/
  case STUDY_WARLOCK_KNOWN_SPELLS_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      display_main_menu(d);
      break;
      /* here are our spell levels for 'spells known' */
    case '1':
    case '2':
    case '3':
    case '4':
      warlock_study_menu(d, atoi(arg));
      break;
    default:
      write_to_output(d, "That is an invalid choice!\r\n");
      warlock_known_spells_disp_menu(d);
      break;
    }
    break;

  case WARLOCK_STUDY_SPELLS:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      warlock_known_spells_disp_menu(d);
      break;

    default:
      number = atoi(arg);

      /* SPELL PREPARATION HOOK */
      for (counter = WARLOCK_POWER_START + 2; counter < WARLOCK_POWER_END; counter++)
      {
        if (counter == number && warlock_spell_type(counter) != WARLOCK_POWER_NONE)
        {
          if (compute_spells_circle(ch, CLASS_WARLOCK,
                                    counter,
                                    METAMAGIC_NONE,
                                    DOMAIN_UNDEFINED) ==
              LEVELUP(d->character)->spell_circle)
          {
            if (*arg2 && is_abbrev(arg2, "help"))
            {
              do_study_spell_help(d->character, counter);
              return;
            }

            if (is_a_known_spell(d->character, CLASS_WARLOCK, counter))
            {
              if (!LEVELUP(d->character)->spells_learned[counter])
              {
                send_to_char(d->character, "\tCYou cannot remove invocation known, unless it is an invocation you already chose this level. "
                                           "To change past choices, you need "
                                           "to respec your character.\r\n\tn");
                break;
              }
              else
              {
                known_spells_remove_by_class(d->character, CLASS_WARLOCK, counter);
                LEVELUP(d->character)->spells_learned[counter] = 0;
              }
            }
            else
            {
              can_add_spell = known_spells_add(d->character, CLASS_WARLOCK, counter, FALSE);
              if (!can_add_spell)
              {
                write_to_output(d, "You are all FULL for invocations!\r\n");
                break;
              }
              else
              {
                LEVELUP(d->character)->spells_learned[counter] = 1;
              }
            }
          }
        }
      }
      warlock_study_menu(d, LEVELUP(d->character)->spell_circle);
      break;
    }
    break;
    /******* end warlock **********/

    /******* start bard **********/

  case STUDY_BARD_KNOWN_SPELLS_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      display_main_menu(d);
      break;

      /* here are our spell levels for 'spells known' */
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
      bard_study_menu(d, atoi(arg));
      break;
    default:
      write_to_output(d, "That is an invalid choice!\r\n");
      bard_known_spells_disp_menu(d);
      break;
    }
    break;

  case BARD_STUDY_SPELLS:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      bard_known_spells_disp_menu(d);
      break;

    default:
      number = atoi(arg);

      /* SPELL PREPARATION HOOK */
      for (counter = 1; counter < NUM_SPELLS; counter++)
      {
        if (counter == number)
        {
          if (compute_spells_circle(ch, CLASS_BARD,
                                    counter,
                                    METAMAGIC_NONE,
                                    DOMAIN_UNDEFINED) ==
              LEVELUP(d->character)->spell_circle)
          {
            if (*arg2 && is_abbrev(arg2, "help"))
            {
              do_study_spell_help(d->character, counter);
              return;
            }

            if (is_a_known_spell(d->character, CLASS_BARD, counter))
            {
              if (!LEVELUP(d->character)->spells_learned[counter])
              {
                send_to_char(d->character, "\tCYou cannot remove spells known, unless it is a spell you already chose this level. "
                                           "To change past choices, you need "
                                           "to respec your character.\r\n\tn");
                break;
              }
              else
              {
                known_spells_remove_by_class(d->character, CLASS_BARD, counter);
                LEVELUP(d->character)->spells_learned[counter] = 0;
              }
            }
            else
            {
              can_add_spell = known_spells_add(d->character, CLASS_BARD, counter, FALSE);
              if (!can_add_spell)
              {
                write_to_output(d, "You are all FULL for spells!\r\n");
                break;
              }
              else
              {
                LEVELUP(d->character)->spells_learned[counter] = 1;
              }
            }
          }
        }
      }
      bard_study_menu(d, LEVELUP(d->character)->spell_circle);
      break;
    }
    break;
    /******* end bard **********/

    /******* start summoner **********/

  case STUDY_SUMMONER_KNOWN_SPELLS_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      display_main_menu(d);
      break;

      /* here are our spell levels for 'spells known' */
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
      summoner_study_menu(d, atoi(arg));
      break;
    default:
      write_to_output(d, "That is an invalid choice!\r\n");
      summoner_known_spells_disp_menu(d);
      break;
    }
    break;

  case SUMMONER_STUDY_SPELLS:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      summoner_known_spells_disp_menu(d);
      break;

    default:
      number = atoi(arg);

      /* SPELL PREPARATION HOOK */
      for (counter = 1; counter < NUM_SPELLS; counter++)
      {
        if (counter == number)
        {
          if (compute_spells_circle(ch, CLASS_SUMMONER,
                                    counter,
                                    METAMAGIC_NONE,
                                    DOMAIN_UNDEFINED) ==
              LEVELUP(d->character)->spell_circle)
          {
            if (*arg2 && is_abbrev(arg2, "help"))
            {
              do_study_spell_help(d->character, counter);
              return;
            }

            if (is_a_known_spell(d->character, CLASS_SUMMONER, counter))
            {
              if (!LEVELUP(d->character)->spells_learned[counter])
              {
                send_to_char(d->character, "\tCYou cannot remove spells known, unless it is a spell you already chose this level. "
                                           "To change past choices, you need to respec your character.\r\n\tn");
                break;
              }
              else
              {
                known_spells_remove_by_class(d->character, CLASS_SUMMONER, counter);
                LEVELUP(d->character)->spells_learned[counter] = 0;
              }
            }
            else
            {
              can_add_spell = known_spells_add(d->character, CLASS_SUMMONER, counter, FALSE);
              if (!can_add_spell)
              {
                write_to_output(d, "You are all FULL for spells!\r\n");
                break;
              }
              else
              {
                LEVELUP(d->character)->spells_learned[counter] = 1;
              }
            }
          }
        }
      }
      summoner_study_menu(d, LEVELUP(d->character)->spell_circle);
      break;
    }
    break;

    case STUDY_MAIN_EIDOLON_MENU:
      switch (*arg)
      {
        case '1': // choose base form
        if (GET_EIDOLON_BASE_FORM(ch) != 0 || LEVELUP(d->character)->eidolon_base_form != 0)
          {
            write_to_output(d, "You have already chosen your eidolon base form.  You will need to respec to change it.\r\n");
            break;
          }
          study_eidolon_base_form_select(d);
          break;
        
        break;
        case '2': // choose evolutions
          if (GET_EIDOLON_BASE_FORM(ch) == 0 && LEVELUP(d->character)->eidolon_base_form == 0)
          {
            write_to_output(d, "You need to choose your base form first.\r\n");
            return;
          }
          study_eidolon_evolutions_select(d);
          break;
        case '3': // choose aspects
          if (GET_EIDOLON_BASE_FORM(ch) == 0 && LEVELUP(d->character)->eidolon_base_form == 0)
          {
            write_to_output(d, "You need to choose your base form first.\r\n");
            return;
          }
          if (GET_EIDOLON_BASE_FORM(ch) == 0 && LEVELUP(d->character)->eidolon_base_form == 0)
          {
            write_to_output(d, "You need to choose your base form first.\r\n");
            return;
          }
          else if (!study_has_aspects_unchosen(d))
          {
            write_to_output(d, "\r\nYou are not currently able to select summoner aspects.\r\n");
            return;
          }
          study_summoner_aspect_select(d);
          break;
        case 'q':
        case 'Q':
          display_main_menu(d);
          break;
        default:
          write_to_output(d, "That is not a valid option.\r\n");
          break;
      }
      break;

    case STUDY_EIDOLON_BASE_FORM_SELECT:
      switch (*arg)
      {
        case 'q':
        case 'Q':
          study_eidolon_main_menu_select(d);
          break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
          study_assign_eidolon_base_form(d->character, atoi(arg));
          write_to_output(d, "\tGEidolon Base Form '%s' Selected.\tn\r\n", eidolon_base_form_names[atoi(arg)]);
          study_eidolon_main_menu_select(d);
          break;
        default:
          send_to_char(ch, "Please choose again: ");
          break;
      }
      
      break;

    case STUDY_SELECT_EVOLUTIONS:
      switch (*arg)
      {
      case 'q':
      case 'Q':
          study_eidolon_main_menu_select(d);
          break;
      }
      if (is_abbrev(arg, "fulllist"))
      {
          for (i = 1; i < NUM_EVOLUTIONS; i++)
          {
            write_to_output(d, "%s%2d) [%2d] %-25s \tn", study_qualifies_for_evolution(d->character, i, false) ? "\tGAv\tn " : "\trUn\tn ", i, evolution_list[i].evolution_points, evolution_list[i].name);
            if ((i % 2) == 1)
          write_to_output(d, "\r\n");
          }
          if ((i % 2) != 1)
            write_to_output(d, "\r\n");
          study_show_evolution_select_bottom_text(d);
          return;
      }
      else if (is_abbrev(arg, "list"))
      {
          counter = 0;
          for (i = 1; i < NUM_EVOLUTIONS; i++)
          {
            if (!study_qualifies_for_evolution(d->character, i, false))
              continue;
            counter++;
            found = TRUE;
            write_to_output(d, "%s%2d) [%2d] %-25s \tn", study_qualifies_for_evolution(d->character, i, false) ? "\tGAv\tn " : "\trUn\tn ", i, evolution_list[i].evolution_points, evolution_list[i].name);
            if ((counter % 2) == 1)
          write_to_output(d, "\r\n");
          }
          if ((counter % 2) != 1)
            write_to_output(d, "\r\n");
          if (!found)
          {
            write_to_output(d, "\tCThere are no available evolutions for you right now.\r\n");
          }
          study_show_evolution_select_bottom_text(d);
          return;
      }
      number = atoi(arg);

      if (number < 1 || number >= NUM_EVOLUTIONS)
      {
          write_to_output(d, "That is not a valid evolution.\r\n");
          return;
      }

      LEVELUP(d->character)->temp_evolution = number;

      study_disp_evolution_confirm(d);
      break;

    case STUDY_SELECT_EVOLUTION_CONFIRM:
      if (!*arg)
      {
          write_to_output(d, "Please select 'Yes' or 'No': ");
          return;
      }
      if (is_abbrev(arg, "Yes") || is_abbrev(arg, "yes") || is_abbrev(arg, "YES"))
      {
          LEVELUP(d->character)->eidolon_evolutions[LEVELUP(d->character)->temp_evolution]++;
          write_to_output(d, "You have selected the '%s' evolution for %d evolution points.\r\n",
                          evolution_list[LEVELUP(d->character)->temp_evolution].name, evolution_list[LEVELUP(d->character)->temp_evolution].evolution_points);
          study_eidolon_evolutions_select(d);
          LEVELUP(d->character)->temp_evolution = 0;
          break;
      }
      else if (is_abbrev(arg, "No") || is_abbrev(arg, "no") || is_abbrev(arg, "NO"))
      {
          study_eidolon_evolutions_select(d);
          write_to_output(d, "Okay, skipping selection of '%s'.\r\n", evolution_list[LEVELUP(d->character)->temp_evolution].name);
          LEVELUP(d->character)->temp_evolution = 0;
      }
      else
      {
          write_to_output(d, "Please select 'Yes' or 'No': ");
          return;
      }
      break;

    case STUDY_SELECT_ASPECT:
      switch (*arg)
      {
      case 'q':
      case 'Q':
          study_eidolon_main_menu_select(d);
          break;
      }
      if (is_abbrev(arg, "list"))
      {
          counter = 0;
          for (i = 1; i < NUM_EVOLUTIONS; i++)
          {
            if (LEVELUP(d->character)->summoner_aspects[i]) continue;    
            if (!evolution_list[i].pc_avail) continue;
            write_to_output(d, "%2d) %-25s \tn", i, evolution_list[i].name);
            if ((counter % 2) == 1)
              write_to_output(d, "\r\n");
            counter++;
          }
          if ((counter % 2) != 1)
            write_to_output(d, "\r\n");
          return;
      }
      number = atoi(arg);

      if (number < 1 || number >= NUM_EVOLUTIONS)
      {
          write_to_output(d, "That is not a valid evolution.\r\n");
          return;
      }

      if (LEVELUP(d->character)->summoner_aspects[i])
      {
          write_to_output(d, "You have already chosen that aspect.\r\n");
          return;
      }

      LEVELUP(d->character)->temp_evolution = number;

      study_disp_aspect_confirm(d);
      break;

    case STUDY_SELECT_ASPECT_CONFIRM:
      if (!*arg)
      {
          write_to_output(d, "Please select 'Yes' or 'No': ");
          return;
      }
      if (is_abbrev(arg, "Yes") || is_abbrev(arg, "yes") || is_abbrev(arg, "YES"))
      {
          LEVELUP(d->character)->summoner_aspects[LEVELUP(d->character)->temp_evolution] = 1;
          write_to_output(d, "You have selected the '%s' aspect.\r\n", evolution_list[LEVELUP(d->character)->temp_evolution].name);
          study_summoner_aspect_select(d);
          LEVELUP(d->character)->temp_evolution = 0;
          break;
      }
      else if (is_abbrev(arg, "No") || is_abbrev(arg, "no") || is_abbrev(arg, "NO"))
      {
          study_summoner_aspect_select(d);
          write_to_output(d, "Okay, skipping selection of '%s'.\r\n", evolution_list[LEVELUP(d->character)->temp_evolution].name);
          LEVELUP(d->character)->temp_evolution = 0;
      }
      else
      {
          write_to_output(d, "Please select 'Yes' or 'No': ");
          return;
      }
      break;

      /******* end summoner **********/

      /******* start inquisitor **********/

        case STUDY_INQUISITOR_KNOWN_SPELLS_MENU:
      switch (*arg)
      {
      case 'q':
      case 'Q':
          display_main_menu(d);
          break;

          /* here are our spell levels for 'spells known' */
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
          inquisitor_study_menu(d, atoi(arg));
          break;
      default:
          write_to_output(d, "That is an invalid choice!\r\n");
          inquisitor_known_spells_disp_menu(d);
          break;
      }
      break;

    case INQUISITOR_STUDY_SPELLS:
      switch (*arg)
      {
      case 'q':
      case 'Q':
          inquisitor_known_spells_disp_menu(d);
          break;

      default:
          number = atoi(arg);

          /* SPELL PREPARATION HOOK */
          for (counter = 1; counter < NUM_SPELLS; counter++)
          {
            if (counter == number)
            {
          if (compute_spells_circle(ch, CLASS_INQUISITOR,
                                    counter,
                                    METAMAGIC_NONE,
                                    DOMAIN_UNDEFINED) ==
              LEVELUP(d->character)->spell_circle)
          {
            if (*arg2 && is_abbrev(arg2, "help"))
            {
              do_study_spell_help(d->character, counter);
              return;
            }

            if (is_a_known_spell(d->character, CLASS_INQUISITOR, counter))
            {
              if (!LEVELUP(d->character)->spells_learned[counter])
              {
                send_to_char(d->character, "\tCYou cannot remove spells known, unless it is a spell you already chose this level. "
                                           "To change past choices, you need "
                                           "to respec your character.\r\n\tn");
                break;
              }
              else
              {
                known_spells_remove_by_class(d->character, CLASS_INQUISITOR, counter);
                LEVELUP(d->character)->spells_learned[counter] = 0;
              }
            }
            else
            {
              can_add_spell = known_spells_add(d->character, CLASS_INQUISITOR, counter, FALSE);
              if (!can_add_spell)
              {
                write_to_output(d, "You are all FULL for spells!\r\n");
                break;
              }
              else
              {
                LEVELUP(d->character)->spells_learned[counter] = 1;
              }
            }
          }
        }
      }
      inquisitor_study_menu(d, LEVELUP(d->character)->spell_circle);
      break;
    }
    break;
    /******* end inquisitor **********/

    /******* start psionicist **********/

  case STUDY_PSIONICIST_KNOWN_POWERS_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      display_main_menu(d);
      break;
    // these are our power level menus
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      if (((CLASS_LEVEL(d->character, CLASS_PSIONICIST) + 1) / 2) < atoi(arg))
      {
        send_to_char(d->character, "You are not yet able to learn psionic powers of that level.\r\n");
        break;
      }
      psionicist_study_menu(d, atoi(arg));
      OLC_MODE(d) = PSIONICIST_STUDY_POWERS;
      break;
    default:
      write_to_output(d, "That is an invalid choice!\r\n");
      psionicist_known_powers_disp_menu(d);
      break;
    }
    break;

  case PSIONICIST_STUDY_POWERS:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      psionicist_known_powers_disp_menu(d);
      break;

    default:
      number = atoi(arg);

      /* SPELL PREPARATION HOOK */
      for (counter = PSIONIC_POWER_START; counter <= PSIONIC_POWER_END; counter++)
      {
        if (counter == number)
        {
          if (compute_powers_circle(CLASS_PSIONICIST, counter, METAMAGIC_NONE) == LEVELUP(d->character)->spell_circle)
          {
            if (*arg2 && is_abbrev(arg2, "help"))
            {
              snprintf(buf, sizeof(buf), "power %s", spell_info[counter].name);
              do_help(d->character, buf, 0, 0);
              return;
            }
            if (is_a_known_spell(d->character, CLASS_PSIONICIST, counter))
            {
              if (!LEVELUP(d->character)->spells_learned[counter])
              {
                send_to_char(d->character, "\tCYou cannot remove powers known, unless it is a power you already chose this level. "
                                           "To change past choices, you need to respec your character.\r\n\tn");
                break;
              }
              else
              {
                known_spells_remove_by_class(d->character, CLASS_PSIONICIST, counter);
                LEVELUP(d->character)->spells_learned[counter] = 0;
              }
            }
            else
            {
              can_add_spell = known_spells_add(d->character, CLASS_PSIONICIST, counter, FALSE);
              if (!can_add_spell)
              {
                write_to_output(d, "You are all FULL for powers!\r\n");
                break;
              }
              else
              {
                LEVELUP(d->character)->spells_learned[counter] = 1;
              }
            }
          }
        }
      }
      psionicist_study_menu(d, LEVELUP(d->character)->spell_circle);
      break;
    }
    break;

    /******* end psionicist **********/

  case SET_1ST_DOMAIN:
    number = atoi(arg);
    if (number < 0)
    {
      write_to_output(d, "Invalid value!  Try again.\r\n");
      OLC_MODE(d) = SET_1ST_DOMAIN;
      set_domain_submenu(d);
      return;
    }
    if (number >= NUM_DOMAINS)
    {
      write_to_output(d, "Invalid value!  Try again.\r\n");
      OLC_MODE(d) = SET_1ST_DOMAIN;
      set_domain_submenu(d);
      return;
    }
    if (GET_2ND_DOMAIN(ch) == number)
    {
      write_to_output(d, "You already have that domain!  Try again.\r\n");
      OLC_MODE(d) = SET_1ST_DOMAIN;
      set_domain_submenu(d);
      return;
    }
    GET_1ST_DOMAIN(ch) = number;
    write_to_output(d, "Choice selected.\r\n");
    print_domain_info(d, number);
    OLC_MODE(d) = STUDY_SET_DOMAINS;
    set_domain_menu(d);
    break;
  case SET_2ND_DOMAIN:
    number = atoi(arg);
    if (number < 0)
    {
      write_to_output(d, "Invalid value!  Try again.\r\n");
      OLC_MODE(d) = SET_2ND_DOMAIN;
      set_domain_submenu(d);
      return;
    }
    if (number >= NUM_DOMAINS)
    {
      write_to_output(d, "Invalid value!  Try again.\r\n");
      OLC_MODE(d) = SET_2ND_DOMAIN;
      set_domain_submenu(d);
      return;
    }
    if (GET_1ST_DOMAIN(ch) == number)
    {
      write_to_output(d, "You already have that domain!  Try again.\r\n");
      OLC_MODE(d) = SET_2ND_DOMAIN;
      set_domain_submenu(d);
      return;
    }
    GET_2ND_DOMAIN(ch) = number;
    write_to_output(d, "Choice selected.\r\n");
    print_domain_info(d, number);
    OLC_MODE(d) = STUDY_SET_DOMAINS;
    set_domain_menu(d);
    break;
  case STUDY_SET_DOMAINS:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      display_main_menu(d);
      break;

    default:
      number = atoi(arg);
      switch (number)
      {
      case 1:
        set_domain_submenu(d);
        OLC_MODE(d) = SET_1ST_DOMAIN;
        return;
      case 2:
        if (!CLASS_LEVEL(ch, CLASS_CLERIC))
        {
          send_to_char(ch, "Only clerics can choose a 2nd domain.");
          return;
        }
        set_domain_submenu(d);
        OLC_MODE(d) = SET_2ND_DOMAIN;
        return;
      default:
        return;
      }
      OLC_MODE(d) = STUDY_SET_DOMAINS;
      set_domain_menu(d);
      break;
    }
    break;

  case SET_SCHOOL:
    number = atoi(arg);
    if (number < 0)
    {
      write_to_output(d, "Invalid value!  Try again.\r\n");
      OLC_MODE(d) = SET_SCHOOL;
      set_school_submenu(d);
      return;
    }
    if (number >= NUM_SCHOOLS)
    {
      write_to_output(d, "Invalid value!  Try again.\r\n");
      OLC_MODE(d) = SET_SCHOOL;
      set_school_submenu(d);
      return;
    }
    GET_SPECIALTY_SCHOOL(ch) = number;
    write_to_output(d, "Choice selected.\r\n");
    OLC_MODE(d) = STUDY_SET_SCHOOL;
    print_school_info(d, number);
    set_school_menu(d);
    break;
  case STUDY_SET_SCHOOL:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      display_main_menu(d);
      break;

    default:
      number = atoi(arg);
      switch (number)
      {
      case 0:
        set_school_submenu(d);
        OLC_MODE(d) = SET_SCHOOL;
        return;
      default:
        break;
      }
      OLC_MODE(d) = STUDY_SET_SCHOOL;
      set_school_menu(d);
      break;
    }
    break;

    /***/
  case SET_PREFERRED_ARCANE:
    number = atoi(arg);
    if (number != CLASS_WIZARD &&
        number != CLASS_SORCERER &&
        number != CLASS_SUMMONER &&
        number != CLASS_BARD)
    {
      write_to_output(d, "Invalid value!  Try again.\r\n");
      OLC_MODE(d) = SET_PREFERRED_ARCANE;
      set_preferred_arcane(d);
      return;
    }
    GET_PREFERRED_ARCANE(ch) = number;
    write_to_output(d, "Choice selected.\r\n");
    OLC_MODE(d) = STUDY_SET_P_CASTER;
    set_preferred_caster(d);
    break;
  case SET_PREFERRED_DIVINE:
    number = atoi(arg);
    if (number != CLASS_DRUID &&
        number != CLASS_INQUISITOR &&
        number != CLASS_CLERIC)
    {
      write_to_output(d, "Invalid value!  Try again.\r\n");
      OLC_MODE(d) = SET_PREFERRED_DIVINE;
      set_preferred_divine(d);
      return;
    }
    GET_PREFERRED_DIVINE(ch) = number;
    write_to_output(d, "Choice selected.\r\n");
    OLC_MODE(d) = STUDY_SET_P_CASTER;
    set_preferred_caster(d);
    break;

  case STUDY_SET_P_CASTER:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      display_main_menu(d);
      break;
    default:
      number = atoi(arg);
      switch (number)
      {
      case 0:
        set_preferred_arcane(d);
        OLC_MODE(d) = SET_PREFERRED_ARCANE;
        return;
      case 1:
        set_preferred_divine(d);
        OLC_MODE(d) = SET_PREFERRED_DIVINE;
        return;
      default:
        break;
      }
      OLC_MODE(d) = STUDY_SET_P_CASTER;
      set_preferred_caster(d);
      break;
    } /* end arg switch */
    break;

  case STUDY_SET_S_BLOODLINE:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      display_main_menu(d);
      break;
    default:
      number = atoi(arg);
      switch (number)
      {
      case 0:
        write_to_output(d, "That is not an eligible option at this time.\r\n");
        set_bloodline_arcane(d);
        OLC_MODE(d) = SET_BLOODLINE_ARCANE;
        return;
      case 1:
        set_bloodline_draconic(d);
        OLC_MODE(d) = SET_BLOODLINE_DRACONIC;
        return;
      case 2:
        LEVELUP(ch)->tempFeat = FEAT_SORCERER_BLOODLINE_FEY;
        OLC_MODE(d) = STUDY_CONFIRM_BLOODLINE;
        write_to_output(d, "\r\n%s%s:\r\n%s%s\r\n\r\n", nrm, feat_list[LEVELUP(ch)->tempFeat].name, nrm, feat_list[LEVELUP(ch)->tempFeat].description);
        write_to_output(d, "Choose the fey bloodline? (Y/N): ");
        return;
      case 3:
        LEVELUP(ch)->tempFeat = FEAT_SORCERER_BLOODLINE_UNDEAD;
        OLC_MODE(d) = STUDY_CONFIRM_BLOODLINE;
        write_to_output(d, "\r\n%s%s:\r\n%s%s\r\n\r\n", nrm, feat_list[LEVELUP(ch)->tempFeat].name, nrm, feat_list[LEVELUP(ch)->tempFeat].description);
        write_to_output(d, "Choose the undead bloodline? (Y/N): ");
        return;
      default:
        write_to_output(d, "That is an invalid choice.  Please choose again: ");
        break;
      }
      set_sorcerer_bloodline(d);
      break;
    } /* end arg switch */
    break;

  case SET_BLOODLINE_DRACONIC:
    number = atoi(arg);
    if (number != DRACONIC_HERITAGE_BLACK &&
        number != DRACONIC_HERITAGE_BLUE &&
        number != DRACONIC_HERITAGE_GREEN &&
        number != DRACONIC_HERITAGE_RED &&
        number != DRACONIC_HERITAGE_WHITE &&
        number != DRACONIC_HERITAGE_BRASS &&
        number != DRACONIC_HERITAGE_BRONZE &&
        number != DRACONIC_HERITAGE_COPPER &&
        number != DRACONIC_HERITAGE_SILVER &&
        number != DRACONIC_HERITAGE_GOLD)
    {
      write_to_output(d, "Invalid value!  Try again.\r\n");
      OLC_MODE(d) = SET_BLOODLINE_DRACONIC;
      set_bloodline_draconic(d);
      return;
    }

    /* Store the feat number in the work area in the data structure. */
    LEVELUP(d->character)->tempFeat = FEAT_SORCERER_BLOODLINE_DRACONIC;
    LEVELUP(ch)->sorcerer_bloodline_subtype = number;
    /* Display the description of the bloodline, and give the player an option. */
    write_to_output(d, "%s%s%s: %s\r\nDraconic Heritage: %s dragon - %s energy type\r\n"
                       "Choose this bloodline and heritage? (y/n) : ",
                    nrm, feat_list[LEVELUP(ch)->tempFeat].name, nrm, feat_list[LEVELUP(ch)->tempFeat].description,
                    DRCHRTLIST_NAME(LEVELUP(ch)->sorcerer_bloodline_subtype),
                    DRCHRT_ENERGY_TYPE(LEVELUP(ch)->sorcerer_bloodline_subtype));
    OLC_MODE(d) = STUDY_CONFIRM_BLOODLINE;
    break;

  case STUDY_DRAGON_RIDER_MENU:
    switch(*arg)
    {
      case '1':
        if (GET_DRAGON_RIDER_DRAGON_TYPE(ch))
        {
          write_to_output(d, "You've already chosen your dragon mount type. To choose a different one you will need to respec.\r\n");
          break;
        }
        show_dragonrider_mount_type(d);
        break;
      case '2':
        if (GET_DRAGON_BOND_TYPE(ch))
        {
          write_to_output(d, "You've already chosen your dragon bond type. To choose a different one you will need to respec.\r\n");
          break;
        }
        show_dragonrider_bond_type(d);
        break;
      case 'q':
      case 'Q':
        display_main_menu(d);
        break;
      default:
        write_to_output(d, "Invalid Choice.\r\n");
        break;
    }
    break;

    case STUDY_DRAGON_RIDER_DRAGON_TYPE:
      number = atoi(arg);

      if (number < 1 || number >= NUM_DRAGON_TYPES)
      {
        send_to_char(ch, "That is not a valid selection.\r\n");
        break;
      }
      if (*arg == 'q' || *arg == 'Q')
      {
        show_dragon_rider_menu(d);
        break;
      }

      LEVELUP(d->character)->dragon_rider_dragon_type = number;
      send_to_char(ch, "Choose the '%s' dragon type with '%s' elemental damage? (Y/N): ",
        DRCHRTLIST_NAME(LEVELUP(ch)->dragon_rider_dragon_type),
        DRCHRT_ENERGY_TYPE(LEVELUP(ch)->dragon_rider_dragon_type));
      OLC_MODE(d) = STUDY_DRAGON_RIDER_DRAGON_TYPE_CONFIRM;
      break;
    
    case STUDY_DRAGON_RIDER_DRAGON_TYPE_CONFIRM:
      switch(*arg)
      {
        case 'y':
        case 'Y':
          write_to_output(d, "You've selected the '%s' dragon mount type with '%s' elemental damage.\r\n",
              DRCHRTLIST_NAME(LEVELUP(ch)->dragon_rider_dragon_type),
              DRCHRT_ENERGY_TYPE(LEVELUP(ch)->dragon_rider_dragon_type));
          show_dragon_rider_menu(d);
          break;
        case 'n':
        case 'N':
          LEVELUP(d->character)->dragon_rider_dragon_type = 0;
          show_dragonrider_mount_type(d);
          break;
        default:
          write_to_output(d, "Invalid Choice.\r\n");
          break;
      }
    break;
  

  case STUDY_DRAGON_RIDER_BOND_TYPE:
    switch (*arg)
    {
      case '1':
        write_to_output(d, "Dragon Champion:\r\n");
        write_to_output(d, "Level 3: (Draconic Strength) +1 hitroll and +2 damroll when riding dragon mount.\r\n");
        write_to_output(d, "Level 7: (Draconic Guardian) 10%% reisist to piercing, slashing and bludgeoning damage when riding dragon mount.\r\n");
        write_to_output(d, "Level 10: (Master of War) Weapon base damage (XdY) doubled when riding dragon mount.\r\n");
        write_to_output(d, "\r\n");
        write_to_output(d, "Enter Y to accept or N to choose another dragon bond: ");
        LEVELUP(d->character)->dragon_rider_bond_type = DRAGON_BOND_CHAMPION;
        OLC_MODE(d) = STUDY_DRAGON_RIDER_BOND_TYPE_CONFIRM;
        break;
      case '2':
        write_to_output(d, "Dragon Scion:\r\n");
        write_to_output(d, "Level 3: (Magic Blade) Melee attacks gain elemental damage based on dragon mount type, when riding dragon mount.\r\n");
        write_to_output(d, "Level 7: (Awakened Mind) Number of Dragoon points are doubled.\r\n");
        write_to_output(d, "Level 10: (Deadly Power) Can sacrifice hit points to restore spell slots.\r\n");
        write_to_output(d, "\r\n");
        write_to_output(d, "Enter Y to accept or N to choose another dragon bond: ");
        LEVELUP(d->character)->dragon_rider_bond_type = DRAGON_BOND_MAGE;
        OLC_MODE(d) = STUDY_DRAGON_RIDER_BOND_TYPE_CONFIRM;
        break;
      case '3':
        write_to_output(d, "Dragon Kin:\r\n");
        write_to_output(d, "Level 3: (Lesser Scales) Armor class increases by 2 when riding dragon mount.\r\n");
        write_to_output(d, "Level 7: (Glory's Call) Can spend a dragoon point to give allies +2 to hitroll and 5d6 to maximum hit points.\r\n");
        write_to_output(d, "Level 10: (Great Charge) Charging while riding dragon mount has a chance to knock enemy prone and charge damage increases by 10.\r\n");
        write_to_output(d, "\r\n");
        write_to_output(d, "Enter Y to accept or N to choose another dragon bond: ");
        LEVELUP(d->character)->dragon_rider_bond_type = DRAGON_BOND_KIN;
        OLC_MODE(d) = STUDY_DRAGON_RIDER_BOND_TYPE_CONFIRM;
        break;
      case 'q':
      case 'Q':
        show_dragon_rider_menu(d);
        break;
      default:
        write_to_output(d, "Invalid Choice.\r\n");
        break;      
    }
    break;

  case STUDY_DRAGON_RIDER_BOND_TYPE_CONFIRM:
      switch(*arg)
      {
        case 'y':
        case 'Y':
          write_to_output(d, "You've selected the '%s' dragon bond.\r\n", dragon_bond_types[LEVELUP(d->character)->dragon_rider_bond_type]);
          show_dragon_rider_menu(d);
          break;
        case 'n':
        case 'N':
          LEVELUP(d->character)->dragon_rider_bond_type = 0;
          show_dragon_rider_bond_menu(d);
          break;
        default:
          write_to_output(d, "Invalid Choice.\r\n");
          break;
      }
    break;

  case SET_BLOODLINE_ARCANE:
    number = atoi(arg);
    if (number <= 0 || number >= NUM_SCHOOLS)
    {
      write_to_output(d, "Invalid value!  Try again.\r\n");
      OLC_MODE(d) = SET_BLOODLINE_ARCANE;
      return;
    }

    /* Store the feat number in the work area in the data structure. */
    LEVELUP(d->character)->tempFeat = FEAT_SORCERER_BLOODLINE_ARCANE;
    LEVELUP(ch)->sorcerer_bloodline_subtype = number;
    /* Display the description of the bloodline, and give the player an option. */
    write_to_output(d, "%s%s%s: %s\r\nArcane Heritage: %s school of arcane magic\r\n"
                       "Choose this bloodline and heritage? (y/n) : ",
                    nrm, feat_list[LEVELUP(ch)->tempFeat].name, nrm, feat_list[LEVELUP(ch)->tempFeat].description,
                    spell_schools[LEVELUP(ch)->sorcerer_bloodline_subtype]);
    OLC_MODE(d) = STUDY_CONFIRM_BLOODLINE;
    break;

  case STUDY_CONFIRM_BLOODLINE:
    switch (*arg)
    {
    case 'n':
    case 'N':
      set_sorcerer_bloodline(d);
      break;
    case 'y':
    case 'Y':
      if (add_levelup_feat(d, LEVELUP(ch)->tempFeat))
      {
        write_to_output(d, "Bloodline %s chosen!\r\n", feat_list[LEVELUP(ch)->tempFeat].name);
        if (LEVELUP(ch)->sorcerer_bloodline_subtype > 0)
        {
          if (LEVELUP(ch)->tempFeat == FEAT_SORCERER_BLOODLINE_DRACONIC)
          {
            write_to_output(d, "You've selected the %s draconic heritage.\r\n",
                            DRCHRTLIST_NAME(LEVELUP(ch)->sorcerer_bloodline_subtype));
          }
          else if (LEVELUP(ch)->tempFeat == FEAT_SORCERER_BLOODLINE_ARCANE)
          {
            write_to_output(d, "You've selected the %s arcane heritage.\r\n",
                            spell_schools[LEVELUP(ch)->sorcerer_bloodline_subtype]);
          }
        }
      }
      display_main_menu(d);
      break;
    }
    break;

  case STUDY_SELECT_RACIAL_ABILITY:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      display_main_menu(d);
      break;
    default:
      number = atoi(arg);
      switch (number)
      {
      case 0:
        if (CAN_CHOOSE_HIGH_ELF_CANTRIP(ch))
        {
          write_to_output(d, "You are not able to select that racial ability.\r\n");
          break;
        }
        select_high_elf_cantrip(d);
        OLC_MODE(d) = SET_HIGH_ELF_CANTRIP;
        return;
      case 1:
        if (CAN_CHOOSE_DRAGONBORN_ANCESTRY(ch))
        {
          write_to_output(d, "You are not able to select that racial ability.\r\n");
          break;
        }
        set_dragonborn_draconic_ancestry(d);
        OLC_MODE(d) = SET_DRAGONBORN_ANCESTRY;
        return;
      default:
        write_to_output(d, "That is an invalid choice.  Please choose again: ");
        break;
      }
      select_racial_abils(d);
      break;
    } /* end arg switch */
    break;

    /*****/

  case SET_HIGH_ELF_CANTRIP:
    number = atoi(arg);
    for (i = 1; i < NUM_SPELLS; i++)
    {
      if (i == SPELL_ENCHANT_ITEM)
        continue;
      if (spell_info[i].min_level[CLASS_WIZARD] == 1 && number == i)
        break;
    }
    if (i >= NUM_SPELLS)
    {
      write_to_output(d, "That is not a valid spell for your high elf cantrip.\r\n");
    }
    else
    {
      LEVELUP(ch)->tempFeat = FEAT_HIGH_ELF_CANTRIP;
      LEVELUP(ch)->high_elf_cantrip = number;
      OLC_MODE(d) = STUDY_CONFIRM_HIGH_ELF_CANTRIP;
      /* Display the spell chosen, and give the player an option. */
      write_to_output(d, "%s%s%s: %s\r\nHigh Elf Cantrip: %s\r\n"
                         "Choose this spell as your high elf cantrip? (y/n) : ",
                      nrm, feat_list[LEVELUP(ch)->tempFeat].name, nrm, feat_list[LEVELUP(ch)->tempFeat].description, spell_info[number].name);
      /* Store the feat number in the work area in the data structure. */
      break;
    }
    break;

  case STUDY_CONFIRM_HIGH_ELF_CANTRIP:
    switch (*arg)
    {
    case 'y':
    case 'Y':
      if (add_levelup_feat(d, LEVELUP(ch)->tempFeat))
      {
        write_to_output(d, "%s chosen!\r\n", feat_list[LEVELUP(ch)->tempFeat].name);
        if (LEVELUP(ch)->high_elf_cantrip > 0)
        {
          if (LEVELUP(ch)->tempFeat == FEAT_HIGH_ELF_CANTRIP)
          {
            write_to_output(d, "You've selected the %s spell for your high elf cantrip.\r\n", spell_info[LEVELUP(ch)->high_elf_cantrip].name);
          }
        }
      }
      display_main_menu(d);
      break;
    default:
      select_high_elf_cantrip(d);
      OLC_MODE(d) = SET_HIGH_ELF_CANTRIP;
      break;
    }
    break;

  case SET_DRAGONBORN_ANCESTRY:
    number = atoi(arg);
    if (number != DRACONIC_HERITAGE_BLACK &&
        number != DRACONIC_HERITAGE_BLUE &&
        number != DRACONIC_HERITAGE_GREEN &&
        number != DRACONIC_HERITAGE_RED &&
        number != DRACONIC_HERITAGE_WHITE &&
        number != DRACONIC_HERITAGE_BRASS &&
        number != DRACONIC_HERITAGE_BRONZE &&
        number != DRACONIC_HERITAGE_COPPER &&
        number != DRACONIC_HERITAGE_SILVER &&
        number != DRACONIC_HERITAGE_GOLD)
    {
      write_to_output(d, "Invalid value!  Try again.\r\n");
      OLC_MODE(d) = SET_DRAGONBORN_ANCESTRY;
      set_dragonborn_draconic_ancestry(d);
      return;
    }

    /* Store the feat number in the work area in the data structure. */
    LEVELUP(d->character)->tempFeat = FEAT_DRAGONBORN_ANCESTRY;
    LEVELUP(ch)->dragonborn_draconic_ancestry = number;
    /* Display the description of the ancestry, and give the player an option. */
    write_to_output(d, "%s%s%s: %s\r\nDraconic Ancestry: %s dragon - %s energy type\r\n"
                       "Choose this draconic ancestry? (y/n) : ",
                    nrm, feat_list[LEVELUP(ch)->tempFeat].name, nrm, feat_list[LEVELUP(ch)->tempFeat].description,
                    DRCHRTLIST_NAME(LEVELUP(ch)->dragonborn_draconic_ancestry),
                    DRCHRT_ENERGY_TYPE(LEVELUP(ch)->dragonborn_draconic_ancestry));
    OLC_MODE(d) = STUDY_CONFIRM_DRAGONBORN_ANCESTRY;
    break;

  case STUDY_CONFIRM_DRAGONBORN_ANCESTRY:
    switch (*arg)
    {
    case 'y':
    case 'Y':
      if (add_levelup_feat(d, LEVELUP(ch)->tempFeat))
      {
        write_to_output(d, "%s chosen!\r\n", feat_list[LEVELUP(ch)->tempFeat].name);
        if (LEVELUP(ch)->dragonborn_draconic_ancestry > 0)
        {
          if (LEVELUP(ch)->tempFeat == FEAT_DRAGONBORN_ANCESTRY)
          {
            write_to_output(d, "You've selected %s dragon as your dragonborn ancestry.\r\n", DRCHRTLIST_NAME(LEVELUP(ch)->dragonborn_draconic_ancestry));
          }
        }
      }
      display_main_menu(d);
      break;
    default:
      set_dragonborn_draconic_ancestry(d);
      OLC_MODE(d) = SET_DRAGONBORN_ANCESTRY;
      break;
    }
    break;

  case STUDY_SET_STATS:
    switch (*arg)
    {
    case 'h':
    case 'H':
      break;
    case 'q':
    case 'Q':
      if (stat_points_left(ch))
      {
        send_to_char(ch, "You must spend all of your stat points before continuing.\r\n");
        break;
      }
      // We're applying racial bonuses now instead of in init_start_char in class.c
      if (!HAS_SET_STATS_STUDY(ch))
      {
        LEVELUP(d->character)->con += get_race_stat(GET_RACE(ch), R_CON_MOD);
        LEVELUP(d->character)->str += get_race_stat(GET_RACE(ch), R_STR_MOD);
        LEVELUP(d->character)->dex += get_race_stat(GET_RACE(ch), R_DEX_MOD);
        LEVELUP(d->character)->inte += get_race_stat(GET_RACE(ch), R_INTEL_MOD);
        LEVELUP(d->character)->wis += get_race_stat(GET_RACE(ch), R_WIS_MOD);
        LEVELUP(d->character)->cha += get_race_stat(GET_RACE(ch), R_CHA_MOD);
        HAS_SET_STATS_STUDY(ch) = TRUE;
      }
      // let's update trains
      intel_bonus += ((GET_REAL_INT(ch) + LEVELUP(ch)->inte) - GET_REAL_INT(ch)) / 2;
      if (GET_LEVEL(ch) == 1)
        intel_bonus *= 4;
      GET_LEVELUP_SKILL_POINTS(ch) += intel_bonus;
      display_main_menu(d);
      break;

    default:
      number = atoi(arg);
      write_to_output(d, "Please enter the value to modify your current stat by."
                         "  Example:  If you want to change your stat from 10 to 14, you would enter "
                         "'4' now.  If you wanted to change your stat from 10 to 8, you would enter '-2' now.\r\n");
      switch (number)
      {
      case 0: /* str */
        OLC_MODE(d) = SET_STAT_STR;
        return;
      case 1: /* dex */
        OLC_MODE(d) = SET_STAT_DEX;
        return;
      case 2: /* con */
        OLC_MODE(d) = SET_STAT_CON;
        return;
      case 3: /* inte */
        OLC_MODE(d) = SET_STAT_INTE;
        return;
      case 4: /* wis */
        OLC_MODE(d) = SET_STAT_WIS;
        return;
      case 5: /* cha */
        OLC_MODE(d) = SET_STAT_CHA;
        return;
      default:
        break;
      }
      OLC_MODE(d) = STUDY_SET_STATS;
      set_stats_menu(d);
      break;
    }
    break;
  /***** end study set stats */
  case SET_STAT_STR:
    number = MAX(-10, MIN(atoi(arg), 10));
    /*debug*/
    // write_to_output(d, "Number: %d\r\n", number);
    points_left = stat_points_left(d->character);
    /*debug*/
    // write_to_output(d, "Points Left: %d\r\n", points_left);
    new_stat = LEVELUP(d->character)->str + number;
    /*debug*/
    // write_to_output(d, "New Stat: %d\r\n", new_stat);
    if (new_stat < compute_base_str(d->character) ||
        new_stat > compute_base_str(d->character) + MAX_POINTS_IN_A_STAT)
    {
      write_to_output(d, "This would put you below/above the stat-cap!\r\n");
      break;
    }
    cost_for_number = compute_str_cost(d->character, number) -
                      compute_str_cost(d->character, 0); /*total cost*/
    /*debug*/
    // write_to_output(d, "Cost for 'Number': %d\r\n", cost_for_number);
    if ((points_left - cost_for_number) >= 0)
    {
      if (new_stat >= compute_base_str(d->character) &&
          new_stat <= (compute_base_str(d->character) + MAX_POINTS_IN_A_STAT))
      {
        /* success! */
        LEVELUP(d->character)->str = new_stat;
        OLC_MODE(d) = STUDY_SET_STATS;
        set_stats_menu(d);
        return;
      }
      else
      {
        write_to_output(d, "That would put you below/above the stat-cap!\r\n");
        break;
      }
    }
    else
    {
      write_to_output(d, "You do not have enough points!\r\n");
      break;
    }
    break;
  case SET_STAT_DEX:
    number = MAX(-10, MIN(atoi(arg), 10));
    /*debug*/
    // write_to_output(d, "Number: %d\r\n", number);
    points_left = stat_points_left(d->character);
    /*debug*/
    // write_to_output(d, "Points Left: %d\r\n", points_left);
    new_stat = LEVELUP(d->character)->dex + number;
    /*debug*/
    // write_to_output(d, "New Stat: %d\r\n", new_stat);
    if (new_stat < compute_base_dex(d->character) ||
        new_stat > compute_base_dex(d->character) + MAX_POINTS_IN_A_STAT)
    {
      write_to_output(d, "This would put you below/above the stat-cap!\r\n");
      break;
    }
    cost_for_number = compute_dex_cost(d->character, number) -
                      compute_dex_cost(d->character, 0);
    /*debug*/
    // write_to_output(d, "Cost for 'Number': %d\r\n", cost_for_number);
    if ((points_left - cost_for_number) >= 0)
    {
      if (new_stat >= compute_base_dex(d->character) &&
          new_stat <= (compute_base_dex(d->character) + MAX_POINTS_IN_A_STAT))
      {
        /* success! */
        LEVELUP(d->character)->dex = new_stat;
        OLC_MODE(d) = STUDY_SET_STATS;
        set_stats_menu(d);
        return;
      }
      else
      {
        write_to_output(d, "That would put you below/above the stat-cap!\r\n");
        break;
      }
    }
    else
    {
      write_to_output(d, "You do not have enough points!\r\n");
      break;
    }
    break;
  case SET_STAT_CON:
    number = MAX(-10, MIN(atoi(arg), 10));
    /*debug*/
    // write_to_output(d, "Number: %d\r\n", number);
    points_left = stat_points_left(d->character);
    /*debug*/
    // write_to_output(d, "Points Left: %d\r\n", points_left);
    new_stat = LEVELUP(d->character)->con + number;
    /*debug*/
    // write_to_output(d, "New Stat: %d\r\n", new_stat);
    if (new_stat < compute_base_con(d->character) ||
        new_stat > compute_base_con(d->character) + MAX_POINTS_IN_A_STAT)
    {
      write_to_output(d, "This would put you below/above the stat-cap!\r\n");
      break;
    }
    cost_for_number = compute_con_cost(d->character, number) -
                      compute_con_cost(d->character, 0);
    /*debug*/
    // write_to_output(d, "Cost for 'Number': %d\r\n", cost_for_number);
    if ((points_left - cost_for_number) >= 0)
    {
      if (new_stat >= compute_base_con(d->character) &&
          new_stat <= (compute_base_con(d->character) + MAX_POINTS_IN_A_STAT))
      {
        /* success! */
        LEVELUP(d->character)->con = new_stat;
        OLC_MODE(d) = STUDY_SET_STATS;
        set_stats_menu(d);
        return;
      }
      else
      {
        write_to_output(d, "That would put you below/above the stat-cap!\r\n");
        break;
      }
    }
    else
    {
      write_to_output(d, "You do not have enough points!\r\n");
      break;
    }
    break;
  case SET_STAT_INTE:
    number = MAX(-10, MIN(atoi(arg), 10));
    /*debug*/
    // write_to_output(d, "Number: %d\r\n", number);
    points_left = stat_points_left(d->character);
    /*debug*/
    // write_to_output(d, "Points Left: %d\r\n", points_left);
    new_stat = LEVELUP(d->character)->inte + number;
    /*debug*/
    // write_to_output(d, "New Stat: %d\r\n", new_stat);
    if (new_stat < compute_base_inte(d->character) ||
        new_stat > compute_base_inte(d->character) + MAX_POINTS_IN_A_STAT)
    {
      write_to_output(d, "This would put you below/above the stat-cap!\r\n");
      break;
    }
    cost_for_number = compute_inte_cost(d->character, number) -
                      compute_inte_cost(d->character, 0);
    /*debug*/
    // write_to_output(d, "Cost for 'Number': %d\r\n", cost_for_number);
    if ((points_left - cost_for_number) >= 0)
    {
      if (new_stat >= compute_base_inte(d->character) &&
          new_stat <= (compute_base_inte(d->character) + MAX_POINTS_IN_A_STAT))
      {
        /* success! */
        LEVELUP(d->character)->inte = new_stat;
        OLC_MODE(d) = STUDY_SET_STATS;
        set_stats_menu(d);
        return;
      }
      else
      {
        write_to_output(d, "That would put you below/above the stat-cap!\r\n");
        break;
      }
    }
    else
    {
      write_to_output(d, "You do not have enough points!\r\n");
      break;
    }
    break;
  case SET_STAT_WIS:
    number = MAX(-10, MIN(atoi(arg), 10));
    /*debug*/
    // write_to_output(d, "Number: %d\r\n", number);
    points_left = stat_points_left(d->character);
    /*debug*/
    // write_to_output(d, "Points Left: %d\r\n", points_left);
    new_stat = LEVELUP(d->character)->wis + number;
    /*debug*/
    // write_to_output(d, "New Stat: %d\r\n", new_stat);
    if (new_stat < compute_base_wis(d->character) ||
        new_stat > compute_base_wis(d->character) + MAX_POINTS_IN_A_STAT)
    {
      write_to_output(d, "This would put you below/above the stat-cap!\r\n");
      break;
    }
    cost_for_number = compute_wis_cost(d->character, number) -
                      compute_wis_cost(d->character, 0);
    /*debug*/
    // write_to_output(d, "Cost for 'Number': %d\r\n", cost_for_number);
    if ((points_left - cost_for_number) >= 0)
    {
      if (new_stat >= compute_base_wis(d->character) &&
          new_stat <= (compute_base_wis(d->character) + MAX_POINTS_IN_A_STAT))
      {
        /* success! */
        LEVELUP(d->character)->wis = new_stat;
        OLC_MODE(d) = STUDY_SET_STATS;
        set_stats_menu(d);
        return;
      }
      else
      {
        write_to_output(d, "That would put you below/above the stat-cap!\r\n");
        break;
      }
    }
    else
    {
      write_to_output(d, "You do not have enough points!\r\n");
      break;
    }
    break;
  case SET_STAT_CHA:
    number = MAX(-10, MIN(atoi(arg), 10));
    /*debug*/
    // write_to_output(d, "Number: %d\r\n", number);
    points_left = stat_points_left(d->character);
    /*debug*/
    // write_to_output(d, "Points Left: %d\r\n", points_left);
    new_stat = LEVELUP(d->character)->cha + number;
    /*debug*/
    // write_to_output(d, "New Stat: %d\r\n", new_stat);
    if (new_stat < compute_base_cha(d->character) ||
        new_stat > compute_base_cha(d->character) + MAX_POINTS_IN_A_STAT)
    {
      write_to_output(d, "This would put you below/above the stat-cap!\r\n");
      break;
    }
    cost_for_number = compute_cha_cost(d->character, number) -
                      compute_cha_cost(d->character, 0);
    /*debug*/
    write_to_output(d, "Cost for 'Number': %d\r\n", cost_for_number);
    if ((points_left - cost_for_number) >= 0)
    {
      if (new_stat >= compute_base_cha(d->character) &&
          new_stat <= (compute_base_cha(d->character) + MAX_POINTS_IN_A_STAT))
      {
        /* success! */
        LEVELUP(d->character)->cha = new_stat;
        OLC_MODE(d) = STUDY_SET_STATS;
        set_stats_menu(d);
        return;
      }
      else
      {
        write_to_output(d, "That would put you below/above the stat-cap!\r\n");
        break;
      }
    }
    else
    {
      write_to_output(d, "You do not have enough points!\r\n");
      break;
    }
    break;

  case FAVORED_ENEMY:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      display_main_menu(d);
      break;

    default:
      number = atoi(arg);
      int ranger_level = CLASS_LEVEL(d->character, CLASS_RANGER);
      switch (number)
      {
      case 0:
        if (ranger_level)
        {
          LEVELUP(d->character)->favored_slot = number;
          favored_enemy_submenu(d, number);
          OLC_MODE(d) = FAVORED_ENEMY_SUB;
          return;
        }
        break;
      case 1:
        if (ranger_level >= 5)
        {
          LEVELUP(d->character)->favored_slot = number;
          favored_enemy_submenu(d, number);
          OLC_MODE(d) = FAVORED_ENEMY_SUB;
          return;
        }
        else
        {
          write_to_output(d, "You are not a high enough level ranger to"
                             " modify this slot!\r\n");
        }
        break;
      case 2:
        if (ranger_level >= 10)
        {
          LEVELUP(d->character)->favored_slot = number;
          favored_enemy_submenu(d, number);
          OLC_MODE(d) = FAVORED_ENEMY_SUB;
          return;
        }
        else
        {
          write_to_output(d, "You are not a high enough level ranger to"
                             " modify this slot!\r\n");
        }
        break;
      case 3:
        if (ranger_level >= 15)
        {
          LEVELUP(d->character)->favored_slot = number;
          favored_enemy_submenu(d, number);
          OLC_MODE(d) = FAVORED_ENEMY_SUB;
          return;
        }
        else
        {
          write_to_output(d, "You are not a high enough level ranger to"
                             " modify this slot!\r\n");
        }
        break;
      case 4:
        if (ranger_level >= 20)
        {
          LEVELUP(d->character)->favored_slot = number;
          favored_enemy_submenu(d, number);
          OLC_MODE(d) = FAVORED_ENEMY_SUB;
          return;
          ;
        }
        else
        {
          write_to_output(d, "You are not a high enough level ranger to"
                             " modify this slot!\r\n");
        }
        break;
      case 5:
        if (ranger_level >= 25)
        {
          LEVELUP(d->character)->favored_slot = number;
          favored_enemy_submenu(d, number);
          OLC_MODE(d) = FAVORED_ENEMY_SUB;
          return;
        }
        else
        {
          write_to_output(d, "You are not a high enough level ranger to"
                             " modify this slot!\r\n");
        }
        break;
      case 6:
        if (ranger_level >= 30)
        {
          LEVELUP(d->character)->favored_slot = number;
          favored_enemy_submenu(d, number);
          OLC_MODE(d) = FAVORED_ENEMY_SUB;
          return;
        }
        else
        {
          write_to_output(d, "You are not a high enough level ranger to"
                             " modify this slot!\r\n");
        }
        break;
      case 7:
      case 8:
      case 9:
        write_to_output(d, "This slot is not currently modifyable.\r\n");
        break;
      }

      OLC_MODE(d) = FAVORED_ENEMY;
      favored_enemy_menu(d);
      break;
    }
    break;

  case FAVORED_ENEMY_SUB:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      favored_enemy_menu(d);
      OLC_MODE(d) = FAVORED_ENEMY;
      break;

    default:
      number = atoi(arg);

      if (number < 0 || number >= NUM_RACE_TYPES)
        write_to_output(d, "Invalid race!\r\n");
      else
      {
        GET_FAVORED_ENEMY(d->character, LEVELUP(d->character)->favored_slot) =
            number;
        favored_enemy_menu(d);
        OLC_MODE(d) = FAVORED_ENEMY;
        return;
      }

      favored_enemy_submenu(d, LEVELUP(d->character)->favored_slot);
      break;
    }
    break;

  case ANIMAL_COMPANION:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      display_main_menu(d);
      break;
    default:
      number = atoi(arg);

      if (number == 0)
      {
        GET_ANIMAL_COMPANION(d->character) = number;
        write_to_output(d, "Your companion has been set to OFF.\r\n");
      }
      else if (number < 0 || number >= NUM_ANIMALS)
      {
        write_to_output(d, "Not a valid choice!\r\n");
      }
      else
      {
        GET_ANIMAL_COMPANION(d->character) = animal_vnums[number];
        write_to_output(d, "You have selected %s.\r\n",
                        animal_names[number]);
      }

      animal_companion_menu(d);
      break;
    }
    break;

  case FAMILIAR_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      display_main_menu(d);
      break;
    default:
      number = atoi(arg);

      if (!number)
      {
        GET_FAMILIAR(d->character) = number;
        write_to_output(d, "Your familiar has been set to OFF.\r\n");
      }
      else if (number < 0 || number >= NUM_FAMILIARS)
      {
        write_to_output(d, "Not a valid choice!\r\n");
      }
      else
      {
        GET_FAMILIAR(d->character) = familiar_vnums[number];
        write_to_output(d, "You have selected %s.\r\n",
                        familiar_names[number]);
      }

      familiar_menu(d);
      break;
    }
    break;

    /* We should never get here, but just in case... */
  default:
    cleanup_olc(d, CLEANUP_CONFIG);
    free(LEVELUP(d->character));
    LEVELUP(d->character) = NULL;
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: study_parse(): Reached default case!");
    write_to_output(d, "Oops...\r\n");
    break;
  }
  /*-------------------------------------------------------------------*/
  /*. END OF CASE */
}

sbyte isSpecialFeat(int feat)
{
  if (isSorcBloodlineFeat(feat))
    return true;
  if (isRacialFeat(feat))
    return true;

  return false;
}

sbyte isRacialFeat(int feat)
{
  switch (feat)
  {
  case FEAT_HIGH_ELF_CANTRIP:
    return true;
  case FEAT_DRAGONBORN_ANCESTRY:
    return true;
  }
  return false;
}

void study_eidolon_base_form_select(struct descriptor_data *d)
{
  int i = 0;

  write_to_output(d, "Choose Your Eidolon Base Form:\r\n\r\n");
  for (i = 1; i < NUM_EIDOLON_BASE_FORMS; i++)
  {
    write_to_output(d, "%d) %-10s: %s\r\n", i, eidolon_base_form_names[i], eidolon_base_form_descs[i]);
  }
  write_to_output(d, "\tnYour Choice: ");
  OLC_MODE(d) = STUDY_EIDOLON_BASE_FORM_SELECT;
}

void study_eidolon_main_menu_select(struct descriptor_data *d)
{
  write_to_output(d, "Summoner Eidolons and Aspects:\r\n"
                     "\r\n"
                     "1) Choose Eidolon Base Form : %s\tn\r\n"
                     "2) Choose Eidolon Evolutions: %s\tn\r\n"
                     "3) Choose Summoner Aspects  : %s\tn\r\n"
                     "Q) Quit to Main Menu\r\n"
                     "\r\n"
                     "\tnYour Choice: ",
                  (LEVELUP(d->character)->eidolon_base_form == 0) ? "Must Choose this First" : 
                  eidolon_base_form_names[LEVELUP(d->character)->eidolon_base_form],
                  has_evolutions_unchosen(d->character) ? "\tGAvailable" : "\trNot Available",
                  study_has_aspects_unchosen(d) ? "\tGAvailable" : "\trNot Available");
  OLC_MODE(d) = STUDY_MAIN_EIDOLON_MENU;
}

int study_num_free_evolution_points(struct char_data *ch)
{
  int num_points = evolution_points[CLASS_LEVEL(ch, CLASS_SUMMONER)] + CLASS_LEVEL(ch, CLASS_NECROMANCER);
  int num_spent = 0, form_evo = 0;
  int i = 0;

  for (i = 1; i < NUM_EVOLUTIONS; i++)
  {
    form_evo = is_eidolon_base_form_evolution(GET_EIDOLON_BASE_FORM(ch) > 0 ? GET_EIDOLON_BASE_FORM(ch) : LEVELUP(ch)->eidolon_base_form, i);
    if (LEVELUP(ch)->eidolon_evolutions[i] > 0)
    {
      num_spent += MAX(0, LEVELUP(ch)->eidolon_evolutions[i] - form_evo) * evolution_list[i].evolution_points;
    }
  }

  return (num_points - num_spent);

}

void study_show_evolution_select_bottom_text(struct descriptor_data *d)
{

  int num_avail = study_num_free_evolution_points(d->character);

  write_to_output(d, "\r\n");
  write_to_output(d, "\r\n");
  write_to_output(d, "Q) Exit from this Menu.\r\n");
  write_to_output(d, "\r\n");
  write_to_output(d, "Evolutions marked \tGAv\tn are eligible to take.\r\n");
  write_to_output(d, "Evolutions marked \trUn\tn are ineligible to take.\r\n");
  write_to_output(d, "Numbers in square brackets (Eg. [4]) are how many evoilution points the ability costs.\r\n");
  write_to_output(d, "Evolution eligibility will update after exiting the entire study menu.\r\n");
  write_to_output(d, "Type \tClist\tn to see the list of available evolutions or \tCfulllist\tn to see a list of all evolutions.\r\n");
  write_to_output(d, "You currently have \tG%d evolution point%s\tn to spend.\r\n", num_avail, num_avail != 1 ? "s" : "");
  write_to_output(d, "Please enter the number of the evolution you wish to take or see information about.\r\nYour Choice: ");
}

void study_show_aspect_select_bottom_text(struct descriptor_data *d)
{

  int num_evos = HAS_REAL_FEAT(d->character, FEAT_ASPECT) + HAS_REAL_FEAT(d->character, FEAT_GREATER_ASPECT) + HAS_REAL_FEAT(d->character, FEAT_EPIC_ASPECT);
  int num_chosen = study_num_aspects_chosen(d);
  int num_avail = MAX(0, num_evos - num_chosen);

  write_to_output(d, "\r\n");
  write_to_output(d, "\r\n");
  write_to_output(d, "Q) Exit from this Menu.\r\n");
  write_to_output(d, "\r\n");
  write_to_output(d, "You are able to choose %d aspect%s\r\n", num_avail, num_avail != 1 ? "s" : "");
  write_to_output(d, "Please enter the number of the aspect you wish to take or see information about.\r\nYour Choice: ");
}

void study_eidolon_evolutions_select(struct descriptor_data *d)
{
  int i = 0, count = 0;
  bool found = FALSE;

  write_to_output(d, "Eidolon Evolutions:\r\n"
                     "\r\n");
  for (i = 1; i < NUM_EVOLUTIONS; i++)
  {
    if (!study_qualifies_for_evolution(d->character, i, false))
      continue;
    count++;
    found = true;
    write_to_output(d, "%s%2d) [%2d] %-25s \tn", study_qualifies_for_evolution(d->character, i, false) ? "\tGAv\tn " : "\trUn\tn ", i, evolution_list[i].evolution_points, evolution_list[i].name);
    if ((count % 2) == 1)
      write_to_output(d, "\r\n");
  }
  if ((count % 2) != 1)
    write_to_output(d, "\r\n");
  if (!found)
    write_to_output(d, "\tCThere are no available evolutions for you right now.\r\n");
  study_show_evolution_select_bottom_text(d);
  OLC_MODE(d) = STUDY_SELECT_EVOLUTIONS;
}



void study_summoner_aspect_select(struct descriptor_data *d)
{
  int i = 0, count = 0;

  if (!study_has_aspects_unchosen(d))
  {
    write_to_output(d, "You do not have any aspects left to choose.  If you have chosen an aspect in error, please exit the study menu without saving and try again.\r\n");
    study_show_aspect_select_bottom_text(d);
    OLC_MODE(d) = STUDY_SELECT_ASPECT;
    return;
  }

  write_to_output(d, "Aspect Evolutions:\r\n"
                     "\r\n");
  for (i = 1; i < NUM_EVOLUTIONS; i++)
  {
    if (LEVELUP(d->character)->summoner_aspects[i]) continue;
    if (!evolution_list[i].pc_avail) continue;
    write_to_output(d, "%2d) %-25s \tn", i, evolution_list[i].name);
    if ((count % 2) == 1)
      write_to_output(d, "\r\n");
    count++;
  }
  if ((count % 2) != 1)
    write_to_output(d, "\r\n");
  study_show_aspect_select_bottom_text(d);
  OLC_MODE(d) = STUDY_SELECT_ASPECT;
}

void study_disp_evolution_confirm(struct descriptor_data *d)
{
  if (!d || !d->character || !LEVELUP(d->character))
    return;

  int evo = LEVELUP(d->character)->temp_evolution;

  display_evolution_info(d->character, evolution_list[evo].name);

  if (!evolution_list[evo].stacks && LEVELUP(d->character)->eidolon_evolutions[evo])
  {
    write_to_output(d, "\r\nYou have already taken this evolution.\r\n");
    return;
  }
  else if (!study_qualifies_for_evolution(d->character, evo, false))
  {
    write_to_output(d, "\r\nYou do not qualify for this evolution.\r\n");
    return;
  }

  write_to_output(d, "\r\nDo you wish to select this evolution? ");
  OLC_MODE(d) = STUDY_SELECT_EVOLUTION_CONFIRM;
}

void study_disp_aspect_confirm(struct descriptor_data *d)
{
  if (!d || !d->character || !LEVELUP(d->character))
    return;

  int evo = LEVELUP(d->character)->temp_evolution;

  display_evolution_info(d->character, evolution_list[evo].name);

  if (LEVELUP(d->character)->summoner_aspects[evo])
  {
    write_to_output(d, "\r\nYou have already taken this evolution.\r\n");
    return;
  }
  if (!evolution_list[evo].pc_avail)
  {
    send_to_char(d->character, "This aspect is not available.\r\n");
    return;
  }
  if (!study_has_aspects_unchosen(d))
  {
    write_to_output(d, "\r\nYou cannot choose more aspects.  If you've chosen in error, please exit the study menu without saving and try again.\r\n");
    return;
  }

  write_to_output(d, "\r\nDo you wish to select this aspect? ");
  OLC_MODE(d) = STUDY_SELECT_ASPECT_CONFIRM;
}

void show_dragonrider_bond_type(struct descriptor_data *d)
{
  if (!d || !d->character || !LEVELUP(d->character))
    return;

  write_to_output(d, "Please select a bond type:\r\n");
  write_to_output(d, "1) Dragon Champion\r\n");
  write_to_output(d, "2) Dragon Scion\r\n");
  write_to_output(d, "3) Dragon Kin\r\n");
  write_to_output(d, "Q) Quit\r\n");
  write_to_output(d, "\r\n");
  write_to_output(d, "Enter Choice: ");

  OLC_MODE(d) = STUDY_DRAGON_RIDER_BOND_TYPE;

}
void show_dragonrider_mount_type(struct descriptor_data *d)
{
  if (!d || !d->character || !LEVELUP(d->character))
    return;

  int i = 0;

  write_to_output(d, "Please select a dragon mount type:\r\n");

  for (i = 1; i < NUM_DRACONIC_HERITAGE_TYPES; i++)
  {
    write_to_output(d, "%2d) %-12s (%s damage type)\r\n", i, draconic_heritage_names[i], damtypes[draconic_heritage_energy_types[i]]);
  }

  write_to_output(d, "Q) Quit\r\n");
  write_to_output(d, "\r\n");
  write_to_output(d, "Enter Choice: ");

  OLC_MODE(d) = STUDY_DRAGON_RIDER_DRAGON_TYPE;

}

/* some undefines from top of file */
#undef C_BEAR
#undef C_BOAR
#undef C_LION
#undef C_CROCODILE
#undef C_HYENA
#undef C_SNOW_LEOPARD
#undef C_SKULL_SPIDER
#undef C_FIRE_BEETLE
#undef C_CAYHOUND
#undef C_DRACAVES
/*--- paladin mount(s) -----*/
#undef C_W_WARHORSE
#undef C_B_DESTRIER
#undef C_STALLION
#undef C_A_DESTRIER
#undef C_G_WARHORSE
#undef C_P_WARHORSE
#undef C_C_DESTRIER
#undef C_WARDOG
#undef C_WARPONY
#undef C_GRIFFON
/*--- familiars -----*/
#undef F_HUNTER
#undef F_PANTHER
#undef F_MOUSE
#undef F_EAGLE
#undef F_RAVEN
#undef F_IMP
#undef F_PIXIE
#undef F_FAERIE_DRAGON
#undef F_PSEUDO_DRAGON
#undef F_HELLHOUND

#undef MENU_OPT
