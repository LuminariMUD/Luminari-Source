
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
#include "domains_schools.h"
#include "assign_wpn_armor.h"
#include "screen.h"

struct domain_info domain_list[NUM_DOMAINS];
struct school_info school_list[NUM_SCHOOLS];

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
  add_domain(DOMAIN_FIRE, "Fire", WEAPON_TYPE_WARHAMMER,
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

    send_to_char(ch, "%sGranted spells: |%s", QCYN, QNRM);
    for (j = 0; j < MAX_DOMAIN_SPELLS; j++) {
      if (domain_list[i].domain_spells[j] != SPELL_RESERVED_DBC) {
        send_to_char(ch, "%s%s|%s", spell_info[domain_list[i].domain_spells[j]].name, QCYN, QNRM);
      }
    }
    send_to_char(ch, "\r\n\r\n");

  }
}