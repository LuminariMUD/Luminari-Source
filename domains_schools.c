
/* ***********************************************************************
 *    File:   domains_schools.c                      Part of LuminariMUD  *
 * Purpose:   Handle loading/assigning of domain and school data          *
 *  Author:   Zusuk                                                       *
 ************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "feats.h"
#include "domains_schools.h"
#include "assign_wpn_armor.h"
#include "screen.h"

struct domain_info domain_list[NUM_DOMAINS];
struct school_info school_list[NUM_SCHOOLS];

/* domain power names */
char *domainpower_names[NUM_DOMAIN_POWERS + 1] = {
  "Undefined",
  "Lightning Arc",
  "Electricity Resistance",
  "Acid Dart",
  "Acid Resistance",
  "Fire Bolt",
  "Fire Resistance",
  "Icicle",
  "Cold Resistance",
  "\n"
};


/* translates whether a given domain power has a corresponding feat */
int domain_power_to_feat(int domain_power) {
  int featnum = FEAT_UNDEFINED;

  switch (domain_power) {
    case DOMAIN_POWER_LIGHTNING_ARC:
      featnum = FEAT_LIGHTNING_ARC;
      break;
    case DOMAIN_POWER_ELECTRICITY_RESISTANCE:
      featnum = FEAT_DOMAIN_ELECTRIC_RESIST;
      break;
    case DOMAIN_POWER_ACID_DART:
      featnum = FEAT_ACID_DART;
      break;
    case DOMAIN_POWER_ACID_RESISTANCE:
      featnum = FEAT_DOMAIN_ACID_RESIST;
      break;
    case DOMAIN_POWER_FIRE_BOLT:
      featnum = FEAT_FIRE_BOLT;
      break;
    case DOMAIN_POWER_FIRE_RESISTANCE:
      featnum = FEAT_DOMAIN_FIRE_RESIST;
      break;
    case DOMAIN_POWER_ICICLE:
      featnum = FEAT_ICICLE;
      break;
    case DOMAIN_POWER_COLD_RESISTANCE:
      featnum = FEAT_DOMAIN_COLD_RESIST;
      break;
    default:
      featnum = FEAT_UNDEFINED;
      break;
  }

  return featnum;
}

/* will clear all the domain feats off a character, used for clerics that
 switch their selected domains */
void clear_domain_feats(struct char_data *ch) {
  int i = 1;

  for (i = 1; i < NUM_FEATS; i++) {
    if (feat_list[i].feat_type == FEAT_TYPE_DOMAIN_ABILITY)
      SET_FEAT(ch, i, 0);
  }
}

/* this will check the two character's selected domains for domain-feats
 and add them to the character's feat list, this assumes you've already
 ran clear_domain_feats() - used for clerics selecting or switching
 their domains */
void add_domain_feats(struct char_data *ch) {
  int i = 0, featnum = FEAT_UNDEFINED;

  for (i = 0; i < NUM_DOMAIN_POWERS; i++) {
    if (has_domain_power(ch, i)) {
      featnum = domain_power_to_feat(i);
      if (featnum != FEAT_UNDEFINED && !HAS_REAL_FEAT(ch, featnum)) {
        SET_FEAT(ch, featnum, 1);
      }
    }
  }
}

int has_domain_power(struct char_data *ch, int domain_power) {
  int i = 0;

  /* we have to loop through the list of powers for each domain the pc has */
  if (GET_1ST_DOMAIN(ch)) {
    for (i = 0; i < MAX_GRANTED_POWERS; i++) {
      if (domain_list[GET_1ST_DOMAIN(ch)].granted_powers[i] == domain_power) {
        return TRUE;
      }
    }
  }

  if (GET_2ND_DOMAIN(ch)) {
    for (i = 0; i < MAX_GRANTED_POWERS; i++) {
      if (domain_list[GET_2ND_DOMAIN(ch)].granted_powers[i] == domain_power) {
        return TRUE;
      }
    }
  }

  return FALSE; /*did not find it!*/
}

/* special spells granted by domains have to be assigned */
void assign_domain_spells(struct char_data *ch) {
  if (!CLASS_LEVEL(ch, CLASS_CLERIC))
    return;

  int i, j, spellnum;

  for (j = 1; j < NUM_DOMAINS; j++) {
    if (GET_1ST_DOMAIN(ch) != j && GET_2ND_DOMAIN(ch) != j)
      continue;
    /* we have this domain */
    for (i = 0; i < MAX_DOMAIN_SPELLS; i++) {
      spellnum = domain_list[j].domain_spells[i];
      if (!GET_SKILL(ch, spellnum))
        SET_SKILL(ch, spellnum, 99);
    }
  }

  return;
}

/* go through and init the list of domains with "empty" values */
void init_domains(void) {
  int i = 0, j = 0;

  for (i = 0; i < NUM_DOMAINS; i++) {
    domain_list[i].name = "None";
    for (j = 0; j < MAX_GRANTED_POWERS; j++)
      domain_list[i].granted_powers[j] = DOMAIN_POWER_UNDEFINED;
    for (j = 0; j < MAX_DOMAIN_SPELLS; j++)
      domain_list[i].domain_spells[j] = SPELL_RESERVED_DBC;
    domain_list[i].favored_weapon = WEAPON_TYPE_UNARMED;
    domain_list[i].description = "None";
  }
}

void add_domain(int domain, char *name, int weapon, char *description) {
  domain_list[domain].name = name;
  domain_list[domain].favored_weapon = weapon;
  domain_list[domain].description = description;
}

void add_domain_powers(int domain, int p1, int p2, int p3, int p4, int p5) {
  domain_list[domain].granted_powers[0] = p1;
  domain_list[domain].granted_powers[1] = p2;
  domain_list[domain].granted_powers[2] = p3;
  domain_list[domain].granted_powers[3] = p4;
  domain_list[domain].granted_powers[4] = p5;
  /* if MAX_GRANTED_POWERS is changed, we have to add it here! */
}

void add_domain_spells(int domain, int s1, int s2, int s3, int s4, int s5,
                       int s6, int s7, int s8, int s9) {
  domain_list[domain].domain_spells[0] = s1;
  domain_list[domain].domain_spells[1] = s2;
  domain_list[domain].domain_spells[2] = s3;
  domain_list[domain].domain_spells[3] = s4;
  domain_list[domain].domain_spells[4] = s5;
  domain_list[domain].domain_spells[5] = s6;
  domain_list[domain].domain_spells[6] = s7;
  domain_list[domain].domain_spells[7] = s8;
  domain_list[domain].domain_spells[8] = s9;
  /* if MAX_DOMAIN_SPELLS is changed, we have to add it here! */
}

void assign_domains(void) {
  /* start by initializing */
  init_domains();

  /* Air Domain */
  add_domain(DOMAIN_AIR, "Air", WEAPON_TYPE_SHORT_SWORD,
      "You can manipulate lightning, mist, and wind, traffic with air creatures, "
        "and are resistant to electricity damage.");
  add_domain_powers(DOMAIN_AIR, DOMAIN_POWER_LIGHTNING_ARC, DOMAIN_POWER_ELECTRICITY_RESISTANCE,
      DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED);
                                /* 1st circle */      /* 2nd circle */
  add_domain_spells(DOMAIN_AIR, SPELL_OBSCURING_MIST, SPELL_EXPEDITIOUS_RETREAT,
      /* 3rd circle */    /* 4th circle */       /* 5th circle */
      SPELL_FLY,          SPELL_STINKING_CLOUD,  SPELL_BILLOWING_CLOUD,
      /* 6th circle */    /* 7th circle */       /* 8th circle */
      SPELL_ACID_FOG,     SPELL_MIRROR_IMAGE,    SPELL_CHAIN_LIGHTNING,
      /* 9th circle */
      SPELL_ELEMENTAL_SWARM);

  /* Earth Domain */
  add_domain(DOMAIN_EARTH, "Earth", WEAPON_TYPE_WARHAMMER,
      "You have mastery over earth, metal, and stone, can fire darts of acid, "
          "and command earth creatures.");
  add_domain_powers(DOMAIN_EARTH, DOMAIN_POWER_ACID_DART, DOMAIN_POWER_ACID_RESISTANCE,
      DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED);
                                /* 1st circle */      /* 2nd circle */
  add_domain_spells(DOMAIN_EARTH, SPELL_IRON_GUTS, SPELL_ACID_ARROW,
      /* 3rd circle */    /* 4th circle */       /* 5th circle */
      SPELL_SLOW,          SPELL_STONESKIN,  SPELL_ACID_SHEATH,
      /* 6th circle */    /* 7th circle */           /* 8th circle */
      SPELL_ACID_FOG,     SPELL_WAVES_OF_EXHAUSTION, SPELL_MASS_DOMINATION,
      /* 9th circle */
      SPELL_ELEMENTAL_SWARM);

  /* Fire Domain */
  add_domain(DOMAIN_FIRE, "Fire", WEAPON_TYPE_BATTLE_AXE,
      "You can call forth fire, command creatures of the inferno, and your "
          "flesh does not burn.");
  add_domain_powers(DOMAIN_FIRE, DOMAIN_POWER_FIRE_BOLT, DOMAIN_POWER_FIRE_RESISTANCE,
      DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED);
                                /* 1st circle */      /* 2nd circle */
  add_domain_spells(DOMAIN_FIRE, SPELL_BURNING_HANDS, SPELL_CONTINUAL_FLAME,
      /* 3rd circle */     /* 4th circle */ /* 5th circle */
      SPELL_SCORCHING_RAY, SPELL_FIREBALL,  SPELL_FIRE_SHIELD,
      /* 6th circle */    /* 7th circle */   /* 8th circle */
      SPELL_FIREBRAND,     SPELL_FIRE_SEEDS, SPELL_FIRE_STORM,
      /* 9th circle */
      SPELL_ELEMENTAL_SWARM);

  /* Water Domain */
  add_domain(DOMAIN_WATER, "Water", WEAPON_TYPE_TRIDENT,
      "You can manipulate water and mist and ice, conjure creatures of water, "
          "and resist cold.");
  add_domain_powers(DOMAIN_WATER, DOMAIN_POWER_ICICLE, DOMAIN_POWER_COLD_RESISTANCE,
      DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED);
                                /* 1st circle */      /* 2nd circle */
  add_domain_spells(DOMAIN_WATER, SPELL_ICE_DAGGER, SPELL_CHILL_TOUCH,
      /* 3rd circle */     /* 4th circle */ /* 5th circle */
      SPELL_BLUR, SPELL_STINKING_CLOUD,  SPELL_COLD_SHIELD,
      /* 6th circle */    /* 7th circle */   /* 8th circle */
      SPELL_ICE_STORM,     SPELL_CONE_OF_COLD, SPELL_DISPLACEMENT,
      /* 9th circle */
      SPELL_ELEMENTAL_SWARM);

  /*unfinished domains*/
  /* Chaos Domain */
  add_domain(DOMAIN_CHAOS, "Chaos", WEAPON_TYPE_DAGGER,
      "(UNFINISHED)Your touch infuses life and weapons with chaos, and you revel in all "
          "things anarchic.");
  add_domain_powers(DOMAIN_CHAOS, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED,
      DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED);
                                  /* 1st circle */    /* 2nd circle */
  add_domain_spells(DOMAIN_CHAOS, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 3rd circle */    /* 4th circle */    /* 5th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 6th circle */    /* 7th circle */    /* 8th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 9th circle */
      SPELL_RESERVED_DBC);
  /* Destruction Domain */
  add_domain(DOMAIN_DESTRUCTION, "Destruction", WEAPON_TYPE_SPEAR,
      "(UNFINISHED)You revel in ruin and devastation, and can deliver particularly "
          "destructive attacks.");
  add_domain_powers(DOMAIN_DESTRUCTION, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED,
      DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED);
                                  /* 1st circle */    /* 2nd circle */
  add_domain_spells(DOMAIN_DESTRUCTION, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 3rd circle */    /* 4th circle */    /* 5th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 6th circle */    /* 7th circle */    /* 8th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 9th circle */
      SPELL_RESERVED_DBC);
  /* Evil Domain */
  add_domain(DOMAIN_EVIL, "Evil", WEAPON_TYPE_SPIKED_CHAIN,
      "(UNFINISHED)You are sinister and cruel, and have wholly pledged your soul to the "
          "cause of evil.");
  add_domain_powers(DOMAIN_EVIL, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED,
      DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED);
                                  /* 1st circle */    /* 2nd circle */
  add_domain_spells(DOMAIN_EVIL, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 3rd circle */    /* 4th circle */    /* 5th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 6th circle */    /* 7th circle */    /* 8th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 9th circle */
      SPELL_RESERVED_DBC);
  /* Good Domain */
  add_domain(DOMAIN_GOOD, "Good", WEAPON_TYPE_SHORT_BOW,
      "(UNFINISHED)You have pledged your life and soul to goodness and purity.");
  add_domain_powers(DOMAIN_GOOD, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED,
      DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED);
                                  /* 1st circle */    /* 2nd circle */
  add_domain_spells(DOMAIN_GOOD, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 3rd circle */    /* 4th circle */    /* 5th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 6th circle */    /* 7th circle */    /* 8th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 9th circle */
      SPELL_RESERVED_DBC);
  /* Healing Domain */
  add_domain(DOMAIN_HEALING, "Healing", WEAPON_TYPE_QUARTERSTAFF,
      "(UNFINISHED)Your touch staves off pain and death, and your healing magic is "
          "particularly vital and potent.");
  add_domain_powers(DOMAIN_HEALING, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED,
      DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED);
                                  /* 1st circle */    /* 2nd circle */
  add_domain_spells(DOMAIN_HEALING, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 3rd circle */    /* 4th circle */    /* 5th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 6th circle */    /* 7th circle */    /* 8th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 9th circle */
      SPELL_RESERVED_DBC);
  /* Knowledge Domain */
  add_domain(DOMAIN_KNOWLEDGE, "Knowledge", WEAPON_TYPE_SICKLE,
      "(UNFINISHED)You are a scholar and a sage of legends. In addition, you treat all "
          "Knowledge skills as class skills.");
  add_domain_powers(DOMAIN_KNOWLEDGE, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED,
      DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED);
                                  /* 1st circle */    /* 2nd circle */
  add_domain_spells(DOMAIN_KNOWLEDGE, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 3rd circle */    /* 4th circle */    /* 5th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 6th circle */    /* 7th circle */    /* 8th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 9th circle */
      SPELL_RESERVED_DBC);
  /* Law Domain */
  add_domain(DOMAIN_LAW, "Law", WEAPON_TYPE_LIGHT_HAMMER,
      "(UNFINISHED)You follow a strict and ordered code of laws, and in so doing, achieve "
          "enlightenment.");
  add_domain_powers(DOMAIN_LAW, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED,
      DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED);
                                  /* 1st circle */    /* 2nd circle */
  add_domain_spells(DOMAIN_LAW, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 3rd circle */    /* 4th circle */    /* 5th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 6th circle */    /* 7th circle */    /* 8th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 9th circle */
      SPELL_RESERVED_DBC);
  /* Trickery Domain */
  add_domain(DOMAIN_TRICKERY, "Trickery", WEAPON_TYPE_SLING,
      "(UNFINISHED)You are a master of illusions and deceptions. Bluff, Disguise, and "
          "Stealth are class skills.");
  add_domain_powers(DOMAIN_TRICKERY, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED,
      DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED);
                                  /* 1st circle */    /* 2nd circle */
  add_domain_spells(DOMAIN_TRICKERY, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 3rd circle */    /* 4th circle */    /* 5th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 6th circle */    /* 7th circle */    /* 8th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 9th circle */
      SPELL_RESERVED_DBC);
  /* Protection Domain */
  add_domain(DOMAIN_PROTECTION, "Protection", WEAPON_TYPE_LIGHT_MACE,
      "(UNFINISHED)Your faith is your greatest source of protection, and you can use that "
          "faith to defend others. In addition, you receive a +1 resistance "
          "bonus on saving throws. This bonus increases by 1 for every 5 levels "
          "you possess.");
  add_domain_powers(DOMAIN_PROTECTION, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED,
      DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED);
                                  /* 1st circle */    /* 2nd circle */
  add_domain_spells(DOMAIN_PROTECTION, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 3rd circle */    /* 4th circle */    /* 5th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 6th circle */    /* 7th circle */    /* 8th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 9th circle */
      SPELL_RESERVED_DBC);
  /* Travel Domain */
  add_domain(DOMAIN_TRAVEL, "Travel", WEAPON_TYPE_SCYTHE,
      "(UNFINISHED)You are an explorer and find enlightenment in the simple joy of travel, "
          "be it by foot or conveyance or magic. Increase your base speed by 10 feet.");
  add_domain_powers(DOMAIN_TRAVEL, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED,
      DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED);
                                  /* 1st circle */    /* 2nd circle */
  add_domain_spells(DOMAIN_TRAVEL, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 3rd circle */    /* 4th circle */    /* 5th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 6th circle */    /* 7th circle */    /* 8th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 9th circle */
      SPELL_RESERVED_DBC);
  /* War Domain */
  add_domain(DOMAIN_WAR, "War", WEAPON_TYPE_LONG_SWORD,
      "(UNFINISHED)You are a crusader for your faith, always ready and willing to fight to defend your faith.");
  add_domain_powers(DOMAIN_WAR, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED,
      DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED, DOMAIN_POWER_UNDEFINED);
                                  /* 1st circle */    /* 2nd circle */
  add_domain_spells(DOMAIN_WAR, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 3rd circle */    /* 4th circle */    /* 5th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 6th circle */    /* 7th circle */    /* 8th circle */
      SPELL_RESERVED_DBC, SPELL_RESERVED_DBC, SPELL_RESERVED_DBC,
      /* 9th circle */
      SPELL_RESERVED_DBC);

  /* end */
  /* this has to be at the end */
  init_domain_spell_level();
}

void domain_spell_level(int spell, int level, int domain) {
  int bad = 0;

  if (spell < 0 || spell > TOP_SPELL_DEFINE) {
    log("SYSERR: attempting assign to illegal domain spellnum %d/%d", spell, TOP_SPELL_DEFINE);
    return;
  }

  if (level < 1 || level > LVL_IMPL) {
    log("SYSERR: assigning domain '%s' to illegal level %d/%d.", skill_name(spell),
            level, LVL_IMPL);
    bad = 1;
  }

  if (!bad)
    spell_info[spell].domain[domain] = level;
}

void init_domain_spell_level(void) {
  int domain, j, spellnum;

  for (domain = 1; domain < NUM_DOMAINS; domain++) {
    for (j = 0; j < MAX_DOMAIN_SPELLS; j++) {
      spellnum = domain_list[domain].domain_spells[j];
      domain_spell_level(spellnum, (j+1)*2-1, domain);
    }
  }

}

ACMD(do_domain) {
  int i = 0, j = 0;;

  for (i = 1; i < NUM_DOMAINS; i++) {
    send_to_char(ch, "%sDomain:%s %-20s %sFavored Weapon:%s %-22s\r\n%sDescription:%s %s\r\n",
                 QCYN, QNRM, domain_list[i].name,
                 QCYN, QNRM, weapon_list[domain_list[i].favored_weapon].name,
                 QCYN, QNRM, domain_list[i].description
                );

    send_to_char(ch, "%sGranted powers: |%s", QCYN, QNRM);
    for (j = 0; j < MAX_GRANTED_POWERS; j++) {
      if (domain_list[i].granted_powers[j] != DOMAIN_POWER_UNDEFINED) {
        send_to_char(ch, "%s%s|%s", domain_power_names[domain_list[i].granted_powers[j]], QCYN, QNRM);
      }
    }
    send_to_char(ch, "\r\n");

    send_to_char(ch, "%sGranted spells: |%s", QCYN, QNRM);
    for (j = 0; j < MAX_DOMAIN_SPELLS; j++) {
      if (domain_list[i].domain_spells[j] != SPELL_RESERVED_DBC) {
        send_to_char(ch, "%s%s|%s", spell_info[domain_list[i].domain_spells[j]].name, QCYN, QNRM);
      }
    }
    send_to_char(ch, "\r\n\r\n");

  }
}