
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

struct domain_info domain_list[NUM_DOMAINS];
struct school_info school_list[NUM_SCHOOLS];

/* special spells granted by domains have to be assigned */
void assign_domain_spell_levels() {

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

  /* end */
}

ACMD(do_domain) {
  int i = 0, j = 0;;

  for (i = 1; i < NUM_DOMAINS; i++) {
    send_to_char(ch, "Domain: %-20s Favored Weapon: %-22s\r\nDescription: %s\r\n",
                 domain_list[i].name,
                 weapon_list[domain_list[i].favored_weapon].name,
                 domain_list[i].description
                );

    send_to_char(ch, "Granted spells: |");
    for (j = 0; j < MAX_DOMAIN_SPELLS; j++) {
      if (domain_list[i].domain_spells[j] != SPELL_RESERVED_DBC) {
        send_to_char(ch, "%s|", spell_info[domain_list[i].domain_spells[j]].name);
      }
    }
    send_to_char(ch, "\r\n");

  }
}