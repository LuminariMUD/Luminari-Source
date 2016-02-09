/*
 LuminariMUD
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "db.h"
#include "race.h"
#include "handler.h"

#define Y   TRUE
#define N   FALSE

struct race_data race_list[NUM_EXTENDED_RACES];
char *race_names[NUM_EXTENDED_RACES];

void set_race_genders(int race, int neuter, int male, int female) {
  race_list[race].genders[0] = neuter;
  race_list[race].genders[1] = male;
  race_list[race].genders[2] = female;
}

void set_race_abilities(int race, int str_mod, int con_mod, int int_mod, int wis_mod, int dex_mod, int cha_mod) {
  race_list[race].ability_mods[0] = str_mod;
  race_list[race].ability_mods[1] = con_mod;
  race_list[race].ability_mods[2] = int_mod;
  race_list[race].ability_mods[3] = wis_mod;
  race_list[race].ability_mods[4] = dex_mod;
  race_list[race].ability_mods[5] = cha_mod;
}

void set_race_height_weight(int race, int neuter_height, int male_height, int female_height, int neuter_weight, int male_weight, int female_weight) {

  race_list[race].height[0] = neuter_height;
  race_list[race].height[1] = male_height;
  race_list[race].height[2] = female_height;
  race_list[race].weight[0] = neuter_weight;
  race_list[race].weight[1] = male_weight;
  race_list[race].weight[2] = female_weight;

}

void set_race_alignments(int race, int lg, int ng, int cg, int ln, int tn, int cn, int le, int ne, int ce) {
  race_list[race].alignments[0] = lg;
  race_list[race].alignments[1] = ng;
  race_list[race].alignments[2] = cg;
  race_list[race].alignments[3] = ln;
  race_list[race].alignments[4] = tn;
  race_list[race].alignments[5] = cn;
  race_list[race].alignments[6] = le;
  race_list[race].alignments[7] = ne;
  race_list[race].alignments[8] = ce;
}

void initialize_races(void) {

  int i = 0;

  for (i = 0; i < NUM_EXTENDED_RACES; i++) {

    race_list[i].name = NULL;
    race_names[i] = NULL;
    race_list[i].abbrev = NULL;
    race_list[i].type = NULL;
    race_list[i].family = RACE_TYPE_UNDEFINED;
    set_race_genders(i, N, N, N);
    race_list[i].menu_display = NULL;
    set_race_height_weight(i, 180, 180, 170, 75, 75, 60);
    set_race_abilities(i, 0, 0, 0, 0, 0, 0);
    race_list[i].size = SIZE_MEDIUM;
    set_race_alignments(i, N, N, N, N, N, N, N, N, N);
    race_list[i].is_pc = FALSE;
    race_list[i].favored_class[0] = CLASS_UNDEFINED;
    race_list[i].favored_class[1] = CLASS_UNDEFINED;
    race_list[i].favored_class[2] = CLASS_UNDEFINED;
    //race_list[i].language = SKILL_LANG_COMMON;
    race_list[i].level_adjustment = 0;
  }
}

/* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod,
 * height-n,height-m,height-f,weight-n,weight-m,weight-f, lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pic, fav-class, lang, lvl-adjust*/
void add_race(int race, char *name, char *abbrev, char *capName, int family, int neuter, int male, int female,
        int str_mod, int con_mod, int int_mod, int wis_mod, int dex_mod, int cha_mod, int height_neuter,
        int height_male, int height_female, int weight_neuter, int weight_male, int weight_female, int lg, int ln, int le,
        int ng, int tn, int ne, int cg, int cn, int ce, int size, int is_pc, int favored_class, int language, int level_adjustment) {
  race_list[race].name = strdup(name);
  race_names[race] = strdup(name);
  race_list[race].abbrev = strdup(abbrev);
  race_list[race].type = strdup(capName);
  //pc_race_types[race] = strdup(capName);
  race_list[race].family = family;
  set_race_genders(race, neuter, male, female);
  set_race_abilities(race, str_mod, con_mod, int_mod, wis_mod, dex_mod, cha_mod);
  set_race_height_weight(race, height_neuter, height_male, height_female, weight_neuter, weight_male, weight_female);
  race_list[race].size = size;
  set_race_alignments(race, lg, ng, cg, ln, tn, cn, le, ne, ce);
  race_list[race].is_pc = is_pc;
  race_list[race].favored_class[0] = favored_class;
  race_list[race].favored_class[1] = favored_class;
  race_list[race].favored_class[2] = favored_class;
  race_list[race].language = language;
  race_list[race].level_adjustment = level_adjustment;
}

void favored_class_female(int race, int favored_class) {
  race_list[race].favored_class[2] = favored_class;
}

void assign_races(void) {

  /* initialization */
  initialize_races();

  /* begin listing */

  /* human */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_HUMAN_NORTHERNER, "northerner", "Northrn", "Northerner", RACE_TYPE_HUMAN, N, Y, Y, 0, 0, 0, 0, 0, 0, 180, 180, 170, 75, 75, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_CHONDATHAN, 0);
  add_race(RACE_HUMAN_CORMYR, "cormyrian", "Cormyr", "Cormyrian", RACE_TYPE_HUMAN, N, Y, Y, 0, -1, 0, 0, 0, 1, 180, 180, 170, 75, 75, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_CHONDATHAN, 0);
  add_race(RACE_HUMAN_ICE_BARBARIAN, "ice barbarian", "IceBarb", "Ice Barbarian", RACE_TYPE_HUMAN, N, Y, Y, 1, 0, 0, 0, 0, -1, 190, 190, 180, 80, 80, 65,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_ICE_BARBARIAN, 0);
  add_race(RACE_HUMAN_AMN, "amnite", "Amnite", "Amnite", RACE_TYPE_HUMAN, N, Y, Y, 0, 0, 0, 0, 1, -1, 180, 180, 170, 75, 75, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_CHONDATHAN, 0);
  add_race(RACE_HUMAN_ANAUROCH, "desert folk", "Desert", "Desert Folk", RACE_TYPE_HUMAN, N, Y, Y, 0, 1, 0, 0, 0, -1, 180, 180, 170, 75, 75, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_MIDANI, 0);
  add_race(RACE_HUMAN_CHULT, "chultian", "Chult", "Chultian", RACE_TYPE_HUMAN, N, Y, Y, 1, 0, -1, 0, 0, 0, 180, 180, 170, 75, 75, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_CHULTAN, 0);
  add_race(RACE_HUMAN_DALELANDS, "daleman", "Daleman", "Daleman", RACE_TYPE_HUMAN, N, Y, Y, 0, 0, 0, 0, 0, 0, 180, 180, 170, 75, 75, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_CHONDATHAN, 0);
  add_race(RACE_HUMAN_HORDELANDS, "hordeman", "Horde", "Hordeman", RACE_TYPE_HUMAN, N, Y, Y, 0, 1, 0, 0, 0, -1, 180, 180, 170, 75, 75, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_TUIGAN, 0);
  add_race(RACE_HUMAN_LANTAN, "lantanite", "Lantan", "Lantanite", RACE_TYPE_HUMAN, N, Y, Y, 0, -1, 1, 0, 0, 0, 180, 180, 170, 75, 75, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_LANTANESE, 0);
  add_race(RACE_HUMAN_MULHORAND, "mulhordanian", "Mulhrnd", "Mulhorandian", RACE_TYPE_HUMAN, N, Y, Y, -1, 0, 1, 0, 0, 0, 180, 180, 170, 75, 75, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_MULHORANDI, 0);
  add_race(RACE_HUMAN_RASHEMAN, "rasheman", "Rashemn", "Rasheman", RACE_TYPE_HUMAN, N, Y, Y, 0, 1, 0, 0, -1, 0, 180, 180, 170, 75, 75, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_RASHEMI, 0);
  add_race(RACE_HUMAN_SEMBIA, "sembian", "Sembian", "Sembian", RACE_TYPE_HUMAN, N, Y, Y, -1, -1, 0, 1, 0, 1, 180, 180, 170, 75, 75, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_CHONDATHAN, 0);
  add_race(RACE_HUMAN_TETHYR, "thethyrian", "Tethyr", "Tethyrian", RACE_TYPE_HUMAN, N, Y, Y, 0, 1, 0, -1, 1, -1, 180, 180, 170, 75, 75, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_COMMON, 0);
  add_race(RACE_HUMAN_THAY, "thayvian", "Thay", "Thayvian", RACE_TYPE_HUMAN, N, Y, Y, 0, -1, 1, 1, 0, -1, 180, 180, 170, 75, 75, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_MULHORANDI, 0);
  add_race(RACE_HUMAN_WATERDEEP, "waterdhavian", "Waterdp", "Waterdhavian", RACE_TYPE_HUMAN, N, Y, Y, 0, -1, 1, 0, -1, 1, 180, 180, 170, 75, 75, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_CHONDATHAN, 0);
  add_race(RACE_HUMAN_SOLAMNIA, "solamnic", "Solmnia", "Solamnic", RACE_TYPE_HUMAN, N, Y, Y, 1, 0, 0, 0, 0, 1, 180, 180, 170, 95, 95, 80,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_SOLAMNIC, 0);
  add_race(RACE_HUMAN_ERGOTH, "ergothian", "Ergoth", "Ergothian", RACE_TYPE_HUMAN, N, Y, Y, 0, 0, 1, 0, 0, 1, 180, 180, 170, 95, 95, 80,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_ERGOT, 0);
  add_race(RACE_HUMAN_ISTAR, "abanasinian", "Abnasin", "Abanasinian", RACE_TYPE_HUMAN, N, Y, Y, 0, 0, 0, 1, 0, 1, 180, 180, 170, 95, 95, 80,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_ISTARIAN, 0);
  add_race(RACE_HUMAN_BALIFOR, "baliforite", "Balifor", "Baliforite", RACE_TYPE_HUMAN, N, Y, Y, 0, 0, 0, 0, 2, 0, 180, 180, 170, 95, 95, 80,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_BALIFORIAN, 0);
  add_race(RACE_HUMAN_KHAROLIS, "kharolisian", "Khrolis", "Kharolisian", RACE_TYPE_HUMAN, N, Y, Y, 1, 0, 0, 0, 1, 0, 180, 180, 170, 95, 95, 80,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_KHAROLISIAN, 0);
  add_race(RACE_HUMAN_NORDMAAR, "nordmaarian", "Nordmar", "Nordmaarian", RACE_TYPE_HUMAN, N, Y, Y, 1, 0, 0, 1, 0, 0, 180, 180, 170, 95, 95, 80,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_NORDMAARIAN, 0);
  add_race(RACE_HUMAN_ICE_FOLK, "ice folk", "IceFolk", "Ice Folk", RACE_TYPE_HUMAN, N, Y, Y, 1, 1, 0, 0, 0, 0, 190, 190, 180, 90, 90, 75,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_ICESPEAK, 0);
  add_race(RACE_HUMAN_PLAINS_NOMAD, "plains barbarian", "PlnBarb", "Plains Barbarian", RACE_TYPE_HUMAN, N, Y, Y, 0, 0, 0, 1, 1, 0, 190, 190, 180, 100,
          100, 85, Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_BARBARIAN, 0);
  add_race(RACE_HUMAN_MOUNTAIN_NOMAD, "mountain barbarian", "MtnBarb", "Mountain Barbarian", RACE_TYPE_HUMAN, N, Y, Y, 2, 0, 0, 0, 0, 0, 190, 190,
          180, 100, 100, 85, Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_BARBARIAN, 0);
  add_race(RACE_HUMAN_DESERT_NOMAD, "desert barbarian", "DstBarb", "Desert Barbarian", RACE_TYPE_HUMAN, N, Y, Y, 0, 2, 0, 0, 0, 0, 190, 190, 180, 100,
          100, 85, Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_BARBARIAN, 0);

  /* elf */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_MOON_ELF, "moon elf", "MoonElf", "Moon Elf", RACE_TYPE_ELF, N, Y, Y, 0, -2, 0, 0, 2, 0, 170, 170, 160, 55, 55, 45,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WIZARD, SKILL_LANG_ELVEN, 0);
  add_race(RACE_SUN_ELF, "sun elf", "SunElf", "Sun Elf", RACE_TYPE_ELF, N, Y, Y, 0, -2, 2, 0, 0, 0, 170, 170, 160, 55, 55, 45,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WIZARD, SKILL_LANG_ELVEN, 0);
  add_race(RACE_WILD_ELF, "wild elf", "WildElf", "Wild Elf", RACE_TYPE_ELF, N, Y, Y, 0, 0, -2, 0, 2, 0, 170, 170, 160, 55, 55, 45,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_SORCERER, SKILL_LANG_ELVEN, 0);
  add_race(RACE_WOOD_ELF, "wood elf", "WoodElf", "Wood Elf", RACE_TYPE_ELF, N, Y, Y, 2, -2, -2, 0, 2, -2, 170, 170, 160, 55, 55, 45,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_RANGER, SKILL_LANG_ELVEN, 0);
  add_race(RACE_DROW_ELF, "drow elf", "DrowElf", "Drow Elf", RACE_TYPE_ELF, N, Y, Y, 0, -2, 2, 0, 2, 2, 170, 170, 160, 55, 55, 45,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WIZARD, SKILL_LANG_UNDERCOMMON, 2);
  favored_class_female(RACE_DROW_ELF, CLASS_CLERIC);
  add_race(RACE_HALF_ELF, "half elf", "HalfElf", "Half Elf", RACE_TYPE_ELF, N, Y, Y, 0, 0, 0, 0, 0, 0, 170, 170, 160, 55, 55, 45,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WIZARD, SKILL_LANG_ELVEN, 0);
  add_race(RACE_HALF_DROW, "half drow", "HlfDrow", "Half Drow", RACE_TYPE_ELF, N, Y, Y, 0, 0, 0, 0, 0, 0, 170, 170, 160, 55, 55, 45,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WIZARD, SKILL_LANG_UNDERCOMMON, 0);
  add_race(RACE_QUALINESTI_ELF, "qualinesti elf", "QualElf", "Qualinesti Elf", RACE_TYPE_ELF, N, Y, Y, 0, -2, 2, 0, 2, 0, 170, 170, 160, 55, 55, 45,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WIZARD, SKILL_LANG_ELVEN, 0);
  add_race(RACE_SILVANESTI_ELF, "silvanesti elf", "SilvElf", "Silvanesti Elf", RACE_TYPE_ELF, N, Y, Y, 0, -2, 2, 2, 2, -2, 170, 170, 160, 55, 55, 45,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WIZARD, SKILL_LANG_ELVEN, 0);
  add_race(RACE_KAGONESTI_ELF, "kagonesti elf", "KagoElf", "Kagonesti Elf", RACE_TYPE_ELF, N, Y, Y, 0, 0, 0, 2, 2, -2, 170, 170, 160, 55, 55, 45,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_RANGER, SKILL_LANG_ELVEN, 0);

  /* dwarf */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_SHIELD_DWARF, "shield dwarf", "ShldDwf", "Shield Dwarf", RACE_TYPE_DWARF, N, Y, Y, 0, 2, 0, 0, 0, -2, 145, 145, 140, 65, 65, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_DWARVEN, 0);
  add_race(RACE_GOLD_DWARF, "gold dwarf", "GoldDwf", "Gold Dwarf", RACE_TYPE_DWARF, N, Y, Y, 0, 2, 0, 0, -2, 0, 145, 145, 140, 65, 65, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_DWARVEN, 0);
  add_race(RACE_GRAY_DWARF, "duergar", "Duergar", "Duergar", RACE_TYPE_DWARF, N, Y, Y, 0, 2, 0, 0, 0, -4, 145, 145, 140, 65, 65, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_UNDERCOMMON, 1);
  add_race(RACE_HYLAR_DWARF, "hylar dwarf", "HylrDwf", "Hylar Dwarf", RACE_TYPE_DWARF, N, Y, Y, 0, 2, 0, 0, 2, -2, 145, 145, 140, 65, 65, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_DWARVEN, 0);
  add_race(RACE_NEIDAR_DWARF, "neidar dwarf", "NidrDwf", "Neidar Dwarf", RACE_TYPE_DWARF, N, Y, Y, 0, 2, 0, 2, 0, -2, 145, 145, 140, 65, 65, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_DWARVEN, 0);
  add_race(RACE_DAEWAR_DWARF, "daewar dwarf", "DawrDwf", "Daewar Dwarf", RACE_TYPE_DWARF, N, Y, Y, 0, 2, 0, 2, 0, -2, 145, 145, 140, 65, 65, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_DWARVEN, 0);
  add_race(RACE_KLAR_DWARF, "klar dwarf", "KlarDwf", "Klar Dwarf", RACE_TYPE_DWARF, N, Y, Y, 0, 2, 0, 2, 0, -2, 145, 145, 140, 65, 65, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_DWARVEN, 0);
  add_race(RACE_DAERGAR_DWARF, "daergar dwarf", "DrgrDwf", "Daergar Dwarf", RACE_TYPE_DWARF, N, Y, Y, 0, 2, 0, 2, 0, -2, 145, 145, 140, 65, 65, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_DWARVEN, 0);
  add_race(RACE_THEIWAR_DWARF, "theiwar dwarf", "ThwrDwf", "Theiwar Dwarf", RACE_TYPE_DWARF, N, Y, Y, 0, 2, 2, 0, 0, -2, 145, 145, 140, 65, 65, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_DWARVEN, 0);
  add_race(RACE_DEWAR_DWARF, "dewar dwarf", "DewrDwf", "Dewar Dwarf", RACE_TYPE_DWARF, N, Y, Y, 0, 2, 0, 0, 2, -0, 145, 145, 140, 65, 65, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_DWARVEN, 0);
  add_race(RACE_AGHAR_DWARF, "gully dwarf", "GulyDwf", "Gully Dwarf", RACE_TYPE_DWARF, N, Y, Y, 0, 4, -4, 2, 4, -2, 145, 145, 140, 65, 65, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_GULLYTALK, 0);
  add_race(RACE_ZHAKAR_DWARF, "zhakar dwarf", "ZhkrDwf", "Zhakar Dwarf", RACE_TYPE_DWARF, N, Y, Y, 0, 2, 0, 2, 0, -2, 145, 145, 140, 65, 65, 60,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_DWARVEN, 0);

  /* halfling */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_LIGHTFOOT_HALFLING, "lightfoot halfling", "LtftHlf", "Lightfoot Halfling", RACE_TYPE_HALFLING, N, Y, Y, -2, 0, 0, 0, 2, 0, 125, 125,
          115, 50, 50, 41, Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_SMALL, TRUE, CLASS_ROGUE, SKILL_LANG_COMMON, 0);
  add_race(RACE_GHOSTWISE_HALFLING, "ghostwise halfling", "GhstHlf", "Ghostwise Halfling", RACE_TYPE_HALFLING, N, Y, Y, -2, 0, 0, 0, 2, 0, 125, 125,
          115, 50, 50, 41, Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_SMALL, TRUE, CLASS_BERSERKER, SKILL_LANG_COMMON, 0);
  add_race(RACE_STRONGHEART_HALFLING, "strongheart halfling", "SthtHlf", "Strongheart Halfling", RACE_TYPE_HALFLING, N, Y, Y, -2, 0, 0, 0, 2, 0, 125,
          125, 115, 50, 50, 41, Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_SMALL, TRUE, CLASS_ROGUE, SKILL_LANG_COMMON, 0);

  /* gnome */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_ROCK_GNOME, "rock gnome", "RkGnome", "Rock Gnome", RACE_TYPE_GNOME, N, Y, Y, -2, 0, 2, 0, 0, 0, 125, 125, 115, 50, 50, 41,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_SMALL, TRUE, CLASS_WIZARD, SKILL_LANG_GNOME, 0);
  add_race(RACE_DEEP_GNOME, "svirfneblin", "Svfnbln", "Svirfneblin", RACE_TYPE_GNOME, N, Y, Y, -2, 0, 0, 2, 2, -4, 125, 125, 115, 50, 50, 41,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_SMALL, TRUE, CLASS_ROGUE, SKILL_LANG_GNOME, 3);
  add_race(RACE_GNOME, "gnome", "Gnome", "Gnome", RACE_TYPE_GNOME, N, Y, Y, 0, 0, 2, -2, 2, 0, 125, 125, 115, 50, 50, 41,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_SMALL, TRUE, CLASS_UNDEFINED, SKILL_LANG_GNOME, 0);

  /* orc */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_HALF_ORC, "half orc", "HalfOrc", "Half Orc", RACE_TYPE_ORC, N, Y, Y, 2, 0, -2, 0, 0, -2, 200, 200, 190, 75, 75, 70,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_BERSERKER, SKILL_LANG_ORCISH, 0);
  add_race(RACE_ORC, "orc", "Orc", "Orc", RACE_TYPE_ORC, N, Y, Y, 2, 2, -2, -2, 0, -2, 210, 210, 200, 85, 85, 80,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_BERSERKER, SKILL_LANG_ORCISH, 0);

  /* monstrous humanoid */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_CENTAUR, "centaur", "Centaur", "Centaur", RACE_TYPE_MONSTROUS_HUMANOID, N, Y, Y, 8, 4, -2, 2, 4, 0, 280, 280, 280, 150, 150, 150,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_LARGE, TRUE, CLASS_RANGER, SKILL_LANG_ELVEN, 3);
  add_race(RACE_IRDA, "irda", "Irda", "Irda", RACE_TYPE_MONSTROUS_HUMANOID, N, Y, Y, 0, 0, 4, 2, 0, 2, 215, 215, 100, 100, 100, 90,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WIZARD, SKILL_LANG_CELESTIAL, 1);
  add_race(RACE_MONSTROUS_HUMANOID, "monstrous humanoid", "MnsHmnd", "Monstrous Humanoid", RACE_TYPE_MONSTROUS_HUMANOID, N, Y, Y, 0, 0, 0, 0, 0, 0, 180,
          180, 180, 65, 65, 55, Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_GOBLIN, "goblin", "Goblin", "Goblin", RACE_TYPE_GOBLINOID, N, Y, Y, -2, 2, 0, 0, 2, -2, 150,
          150, 150, 45, 45, 40, Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_SMALL, FALSE, CLASS_WARRIOR, SKILL_LANG_GOBLIN, 0);
  add_race(RACE_HALF_TROLL, "troll", "Troll", "Troll", RACE_TYPE_GIANT, N, Y, Y, 2, 4, 0, 0, 0, -2, 220,
          220, 220, 75, 75, 70, Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_GOBLIN, 0);

  /* kender */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_KENDER, "kender", "Kender", "Kender", RACE_TYPE_KENDER, N, Y, Y, -2, 0, 0, 0, 4, 0, 125, 125,
          115, 50, 50, 41, Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_SMALL, TRUE, CLASS_ROGUE, SKILL_LANG_KENDERSPEAK, 0);

  /* minotaur */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_MINOTAUR, "minotaur", "Minotar", "Minotaur", RACE_TYPE_MINOTAUR, N, Y, Y, 4, 0, 0, 0, 0, -2, 215, 215, 200, 100, 100, 90,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_KOTHIAN, 0);

  /* giant */
  add_race(RACE_HALF_OGRE, "half ogre", "HlfOgre", "Half Ogre", RACE_TYPE_GIANT, N, Y, Y, 6, 4, -2, 0, 2, 0, 415, 415, 250, 250, 100, 90,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_LARGE, TRUE, CLASS_BERSERKER, SKILL_LANG_GIANT, 2);
  add_race(RACE_GIANT, "giant", "Giant", "Giant", RACE_TYPE_GIANT, N, Y, Y, 14, 6, -4, -4, 0, -4, 500, 500, 500, 200, 200, 200,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 4);

  /* undead */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_UNDEAD, "undead", "Undead", "Undead", RACE_TYPE_UNDEAD, Y, N, N, 0, 0, 0, 0, 0, 0, 180, 180, 180, 75, 75, 65,
          N, N, N, N, Y, N, Y, Y, Y, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_SKELETON, "skeleton", "Skeletn", "Skeleton", RACE_TYPE_UNDEAD, Y, N, N, 0, 0, 0, 0, 0, 0, 180, 180, 170, 35, 35, 35,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_ZOMBIE, "zombie", "Zombie", "Zombie", RACE_TYPE_UNDEAD, N, Y, Y, 0, 0, 0, 0, 0, 0, 180, 180, 170, 75, 75, 65,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_GHOUL, "ghoul", "Ghoul", "Ghoul", RACE_TYPE_UNDEAD, N, Y, Y, 0, 0, 0, 0, 0, 0, 180, 180, 170, 75, 75, 65,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_GHAST, "ghast", "Ghast", "Ghast", RACE_TYPE_UNDEAD, N, Y, Y, 0, 0, 0, 0, 0, 0, 180, 180, 170, 75, 75, 65,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MUMMY, "mummy", "Mummy", "Mummy", RACE_TYPE_UNDEAD, N, Y, Y, 14, 0, 0, 0, 0, 0, 180, 180, 170, 75, 75, 65,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MOHRG, "mohrg", "Mohrg", "Mohrg", RACE_TYPE_UNDEAD, N, Y, Y, 11, 0, 0, 0, 9, 0, 180, 180, 170, 75, 75, 65,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  /* animal */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_EAGLE, "eagle", "Eagle", "Eagle", RACE_TYPE_ANIMAL, N, Y, Y, 0, 2, 0, 0, 5, 0, 10, 10, 10, 10, 10, 10,
          N, N, N, N, Y, N, N, N, N, SIZE_SMALL, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_RAT, "rat", "Rat", "Rat", RACE_TYPE_ANIMAL, N, Y, Y, -8, 0, -2, 0, 4, -2, 50, 50, 50, 25, 25, 25,
          N, N, N, N, Y, N, N, N, N, SIZE_TINY, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_WOLF, "wolf", "Wolf", "Wolf", RACE_TYPE_ANIMAL, N, Y, Y, 2, 4, 0, 0, 4, 0, 65, 65, 65, 45, 45, 40,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_GREAT_CAT, "great cat", "Grt Cat", "Great Cat", RACE_TYPE_ANIMAL, N, Y, Y, 4, 2, 0, 0, 2, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_HORSE, "horse", "Horse", "Horse", RACE_TYPE_ANIMAL, N, Y, Y, 6, 4, 0, 0, 2, 0, 200, 200, 200, 150, 150, 150,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_DINOSAUR, "dinosaur", "Dino", "Dinosaur", RACE_TYPE_ANIMAL, N, Y, Y, 10, 0, 0, 0, 0, 0, 600, 600, 600, 350, 350, 350,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_LION, "lion", "Lion", "Lion", RACE_TYPE_ANIMAL, N, Y, Y, 10, 4, 0, 0, 6, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_BLACK_BEAR, "black bear", "BlkBear", "Black Bear", RACE_TYPE_ANIMAL, N, Y, Y, 8, 4, 0, 0, 2, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_BROWN_BEAR, "brown bear", "BrnBear", "Brown Bear", RACE_TYPE_ANIMAL, N, Y, Y, 16, 8, 0, 0, 2, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_POLAR_BEAR, "polar bear", "PlrBear", "Polar Bear", RACE_TYPE_ANIMAL, N, Y, Y, 16, 8, 0, 0, 2, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_TIGER, "tiger", "Tiger", "Tiger", RACE_TYPE_ANIMAL, N, Y, Y, 12, 6, 0, 0, 4, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_CONSTRICTOR_SNAKE, "constrictor snake", "CnsSnak", "Constrictor Snake", RACE_TYPE_ANIMAL, N, Y, Y, 6, 2, 0, 0, 6, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_GIANT_CONSTRICTOR_SNAKE, "giant constrictor snake", "GCnSnak", "Giant Constrictor Snake", RACE_TYPE_ANIMAL, N, Y, Y, 14, 2, 0, 0, 6, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MEDIUM_VIPER, "medium viper", "MdViper", "Medium Viper", RACE_TYPE_ANIMAL, N, Y, Y, -2, 0, 0, 0, 6, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_LARGE_VIPER, "large viper", "LgViper", "Large Viper", RACE_TYPE_ANIMAL, N, Y, Y, 0, 0, 0, 0, 6, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_HUGE_VIPER, "huge viper", "HgViper", "Huge Viper", RACE_TYPE_ANIMAL, N, Y, Y, 6, 2, 0, 0, 4, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_WOLVERINE, "wolverine", "Wlvrine", "Wolverine", RACE_TYPE_ANIMAL, N, Y, Y, 4, 8, 0, 0, 4, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_RHINOCEROS, "rhinoceros", "Rhino", "Rhinoceros", RACE_TYPE_ANIMAL, N, Y, Y, 16, 10, 0, 0, 0, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_LEOPARD, "leopard", "Leopard", "Leopard", RACE_TYPE_ANIMAL, N, Y, Y, 6, 4, 0, 0, 8, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_HYENA, "hyena", "Hyena", "Hyena", RACE_TYPE_ANIMAL, N, Y, Y, 4, 4, 0, 0, 4, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_CROCODILE, "crocodile", "Crocodl", "Crocodile", RACE_TYPE_ANIMAL, N, Y, Y, 8, 6, 0, 0, 2, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_GIANT_CROCODILE, "giant crocodile", "GCrocdl", "Giant Crocodile", RACE_TYPE_ANIMAL, N, Y, Y, 16, 8, 0, 0, 2, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_CHEETAH, "cheetah", "Cheetah", "Cheetah", RACE_TYPE_ANIMAL, N, Y, Y, 6, 4, 0, 0, 8, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_BOAR, "boar", "Boar", "Boar", RACE_TYPE_ANIMAL, N, Y, Y, 4, 10, 0, 0, 0, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_APE, "ape", "Ape", "Ape", RACE_TYPE_ANIMAL, N, Y, Y, 10, 6, 0, 0, 6, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_ELEPHANT, "elephant", "Elephnt", "Elephant", RACE_TYPE_ANIMAL, N, Y, Y, 20, 10, 0, 0, 0, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  /* plant */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_MANDRAGORA, "mandragora", "Mndrgra", "Mandragora", RACE_TYPE_PLANT, Y, Y, Y, 0, 0, 0, 0, 0, 0, 180, 180, 180, 90, 90, 90,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_SMALL, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MYCANOID, "mycanoid", "Mycanid", "Mycanoid", RACE_TYPE_PLANT, Y, Y, Y, 0, 0, 0, 0, 0, 0, 180, 180, 180, 90, 90, 90,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_SHAMBLING_MOUND, "shambling mound", "Shmbler", "Shambling Mound", RACE_TYPE_PLANT, Y, Y, Y, 0, 0, 0, 0, 0, 0, 180, 180, 180,
          90, 90, 90,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_TREANT, "treant", "Treant", "Treant", RACE_TYPE_PLANT, Y, Y, Y, 0, 0, 0, 0, 0, 0, 180, 180, 180, 90, 90, 90,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  /* ooze */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_OOZE, "ooze", "Ooze", "Ooze", RACE_TYPE_OOZE, Y, N, N, 0, 0, 0, 0, 0, 0, 20, 20, 20, 30, 30, 30,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  /* elemental */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_SMALL_FIRE_ELEMENTAL, "small fire elemental", "SFirElm", "Small Fire Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 4, 6, 0, 0, 10, 0, 85,
          85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_SMALL, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MEDIUM_FIRE_ELEMENTAL, "medium fire elemental", "MFirElm", "Medium Fire Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 4, 6, 0, 0, 10, 0,
          85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_LARGE_FIRE_ELEMENTAL, "large fire elemental", "LFirElm", "Large Fire Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 4, 6, 0, 0, 10, 0, 85,
          85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_HUGE_FIRE_ELEMENTAL, "huge fire elemental", "HFirElm", "Huge Fire Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 8, 8, 0, 0, 14, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  add_race(RACE_SMALL_EARTH_ELEMENTAL, "small earth elemental", "SErtElm", "Small Earth Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 14, 8, 0, 0, -2, 0,
          85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_SMALL, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MEDIUM_EARTH_ELEMENTAL, "medium earth elemental", "MErtElm", "Medium Earth Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 14, 8, 0, 0, -2,
          0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_LARGE_EARTH_ELEMENTAL, "large earth elemental", "LErtElm", "Large Earth Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 14, 8, 0, 0, -2, 0,
          85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_HUGE_EARTH_ELEMENTAL, "huge earth elemental", "HErtElm", "Huge Earth Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 18, 10, 0, 0, -2, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  add_race(RACE_SMALL_AIR_ELEMENTAL, "small air elemental", "SAirElm", "Small Air Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 4, 6, 0, 0, 14, 0, 85, 85,
          80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_SMALL, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MEDIUM_AIR_ELEMENTAL, "medium air elemental", "MAirElm", "Medium Air Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 4, 6, 0, 0, 14, 0, 85,
          85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_LARGE_AIR_ELEMENTAL, "large air elemental", "LAirElm", "Large Air Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 4, 6, 0, 0, 14, 0, 85, 85,
          80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_HUGE_AIR_ELEMENTAL, "huge air elemental", "HAirElm", "Huge Air Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 8, 8, 0, 0, 18, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  add_race(RACE_SMALL_WATER_ELEMENTAL, "small water elemental", "SWatElm", "Small Water Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 10, 8, 0, 0, 4, 0,
          85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_SMALL, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MEDIUM_WATER_ELEMENTAL, "medium water elemental", "MWatElm", "Medium Water Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 10, 8, 0, 0, 4, 0,
          85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_LARGE_WATER_ELEMENTAL, "large water elemental", "LWatElm", "Large Water Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 10, 8, 0, 0, 4, 0,
          85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_HUGE_WATER_ELEMENTAL, "huge water elemental", "HWatElm", "Huge Water Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 14, 10, 0, 0, 8, 0, 85, 85, 80, 60, 60, 55,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  /* magical beast */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  /*
  add_race(RACE_STIRGE, "stirge", "Stirge", "Stirge", RACE_TYPE_MAGICAL_BEAST, N, Y, Y, 0, 0, 0, 0, 0, 0, 100, 100, 100, 55, 55, 55,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_SHOCKER_LIZARD, "shocker lizard", "ShckLiz", "Shocker Lizard", RACE_TYPE_MAGICAL_BEAST, N, Y, Y, 0, 0, 0, 0, 0, 0, 100, 100, 100, 55, 55, 55,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_OWLBEAR, "owlbear", "Owlbear", "Owlbear", RACE_TYPE_MAGICAL_BEAST, N, Y, Y, 0, 0, 0, 0, 0, 0, 100, 100, 100, 55, 55, 55,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  */
  add_race(RACE_BLINK_DOG, "blink dog", "BlnkDog", "Blink Dog", RACE_TYPE_MAGICAL_BEAST, N, Y, Y, 0, 0, 0, 0, 0, 0, 100, 100, 100, 55, 55, 55,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  /* fey */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_PIXIE, "pixie", "Pixie", "Pixie", RACE_TYPE_FEY, N, Y, Y, -4, 0, 4, 0, 4, 4, 80, 80, 80, 20, 20, 20,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_TINY, FALSE, CLASS_DRUID, SKILL_LANG_ELVEN, 3);
  
  /* construct */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_IRON_GOLEM, "construct", "Consrct", "Construct", RACE_TYPE_CONSTRUCT, Y, N, N, 0, 0, 0, 0, 0, 0, 180, 180, 180, 150, 150, 130,
          N, N, N, N, Y, N, Y, Y, Y, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  /* outsiders */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  /*
  add_race(RACE_AIR_GENESI, "air genesi", "AirGnsi", "Air Genesi", RACE_TYPE_OUTSIDER, N, Y, Y, 0, 0, 2, -2, 2, -2, 185, 185, 175, 80, 80, 75,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_CELESTIAL, 1);
  add_race(RACE_FIRE_GENESI, "fire genesi", "FirGnsi", "Fire Genesi", RACE_TYPE_OUTSIDER, N, Y, Y, 0, 0, 2, 0, 0, -2, 185, 185, 175, 80, 80, 75,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_ABYSSAL, 1);
  add_race(RACE_WATER_GENESI, "water genesi", "WatGnsi", "Water Genesi", RACE_TYPE_OUTSIDER, N, Y, Y, 0, 2, 0, 0, 0, -2, 185, 185, 175, 80, 80, 75,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_CELESTIAL, 1);
  add_race(RACE_EARTH_GENESI, "earth genesi", "ErtGnsi", "Earth Genesi", RACE_TYPE_OUTSIDER, N, Y, Y, 2, 2, 0, -2, 0, -2, 185, 185, 175, 80, 80, 75,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_ABYSSAL, 1);
  add_race(RACE_AASIMAR, "aasimar", "Aasimar", "Aasimar", RACE_TYPE_OUTSIDER, N, Y, Y, 0, 0, 0, 2, 0, 2, 185, 185, 175, 80, 80, 75,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_CELESTIAL, 1);
  add_race(RACE_TIEFLING, "tiefling", "Tieflng", "Tiefling", RACE_TYPE_OUTSIDER, N, Y, Y, 0, 0, 2, 0, 2, -2, 185, 185, 175, 80, 80, 75,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_ABYSSAL, 1);
  */
  add_race(RACE_AEON_THELETOS, "aeon theletos", "AeonThel", "Theletos Aeon", RACE_TYPE_OUTSIDER, N, Y, Y, 0, 0, 0, 0, 0, 0, 180, 180, 180, 75, 75, 75,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  /* dragon */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod, height-n,height-m,height-f,weight-n,weight-m,weight-f,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  /*
  add_race(RACE_HALF_DRAGON, "half dragon", "HlfDrgn", "Half Dragon", RACE_TYPE_DRAGON, N, Y, Y, 0, 0, 0, 0, 0, 0, 180, 180, 180, 150, 150, 130,
          N, N, N, N, Y, N, Y, Y, Y, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_DRACONIC, 0);
  add_race(RACE_BAAZ_DRACONIAN, "baaz draconian", "Baaz", "Baaz Draconian", RACE_TYPE_DRAGON, N, Y, Y, 4, 4, 0, 0, 0, 0, 215, 215, 150, 150, 100, 90,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_DRACONIC, 1);
  add_race(RACE_KAPAK_DRACONIAN, "kapak draconian", "Kapak", "Kapak Draconian", RACE_TYPE_DRAGON, N, Y, Y, 0, 2, 2, 0, 4, 0, 215, 215, 150, 150, 100, 90,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_ROGUE, SKILL_LANG_DRACONIC, 2);
  */
  add_race(RACE_DRAGON_CLOUD, "dragon cloud", "DrgCloud", "Cloud Dragon", RACE_TYPE_DRAGON, N, Y, Y, 0, 0, 0, 0, 0, 0, 180, 180, 180, 150, 150, 130,
          N, N, N, N, Y, N, Y, Y, Y, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_DRACONIC, 0);
  
  /* end listing */
}

void racial_ability_modifiers(struct char_data *ch) {
  int chrace = 0;
  if (GET_RACE(ch) >= NUM_EXTENDED_RACES || GET_RACE(ch) < 0) {
    log("SYSERR: Unknown race %d in racial_ability_modifiers", GET_RACE(ch));
  } else {
    chrace = GET_RACE(ch);
  }

  ch->real_abils.str += race_list[chrace].ability_mods[0];
  ch->real_abils.con += race_list[chrace].ability_mods[1];
  ch->real_abils.intel += race_list[chrace].ability_mods[2];
  ch->real_abils.wis += race_list[chrace].ability_mods[3];
  ch->real_abils.dex += race_list[chrace].ability_mods[4];
  ch->real_abils.cha += race_list[chrace].ability_mods[5];
}

void set_height_and_weight_by_race(struct char_data *ch) {
  int race, sex, mod;

  race = GET_RACE(ch);
  sex = GET_SEX(ch);
  if (sex < SEX_NEUTRAL || sex >= NUM_SEX) {
    log("Invalid gender in set_height_and_weight_by_race: %d", sex);
    sex = SEX_NEUTRAL;
  }
  if (race <= RACE_UNDEFINED || race >= NUM_EXTENDED_RACES) {
    log("Invalid gender in set_height_and_weight_by_race: %d", GET_SEX(ch));
    race = RACE_UNDEFINED + 1; /* first defined race */
  }
  mod = 89 + dice(1, 21);
  GET_HEIGHT(ch) = race_list[race].height[sex] * mod / 100;
  GET_WEIGHT(ch) = race_list[race].weight[sex] * mod / 100;

}
/*
int invalid_race(struct char_data *ch, struct obj_data *obj)
{
  if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT)
    return false;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_HUMAN) && IS_HUMAN(ch))
    return (true);

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ELF) && IS_ELF(ch))
    return (true);

  if (OBJ_FLAGGED(obj, ITEM_ANTI_DWARF) && IS_DWARF(ch))
    return (true);

  if (OBJ_FLAGGED(obj, ITEM_ANTI_GNOME) && IS_GNOME(ch))
    return (true);

  return (false);
}
 */

/*
int get_size(struct char_data *ch) {
  int racenum;

  if (ch == NULL)
    return SIZE_MEDIUM;

  racenum = GET_RACE(ch);

  if (racenum < 0 || racenum >= NUM_EXTENDED_RACES)
    return SIZE_MEDIUM;

  return (GET_SIZE(ch) = ((affected_by_spell(ch, SPELL_ENLARGE_PERSON) ? 1 : 0) + race_list[racenum].size));
}
 */

const int size_bonus_table[NUM_SIZES] = {
  /* XTINY */ 8,
  /* TINY */ 4,
  /* XSMALL */ 2,
  /* SMALL */ 1,
  /* MEDIUM */ 0,
  /* LARGE */ -1,
  /* HUGE */ -2,
  /* GIGANTIC */ -4,
  /* COLOSSAL */ -8
};

int get_size_bonus(int sz) {
  if (sz < 0 || sz >= NUM_SIZES)
    sz = SIZE_MEDIUM;
  return size_bonus_table[sz];
}

/*
int wield_type(int chsize, const struct obj_data *weap)
{
  if (GET_OBJ_TYPE(weap) != ITEM_WEAPON) {
    return OBJ_FLAGGED(weap, ITEM_2H) ? WIELD_TWOHAND : WIELD_ONEHAND;
  } else if (chsize > GET_OBJ_SIZE(weap)) {
    return WIELD_LIGHT;
  } else if (chsize == GET_OBJ_SIZE(weap)) {
    return WIELD_ONEHAND;
  } else if (chsize == GET_OBJ_SIZE(weap) - 1) {
    return WIELD_TWOHAND;
  } else if (chsize < GET_OBJ_SIZE(weap) - 1) {
    return WIELD_NONE; // It's just too big for you!
  } else {
    log("unknown size vector in wield_type: chsize=%d, weapsize=%d", chsize, GET_OBJ_SIZE(weap));
    return WIELD_NONE;
  }
}
 */

/*
int parse_race(struct char_data *ch, char *arg) {

  if (!*arg)
    return RACE_UNDEFINED;

  int i = atoi(arg);

  i--;

  if (i > NUM_EXTENDED_RACES || i < RACE_HUMAN)
    return RACE_UNDEFINED;

  if (race_list[i].is_pc == FALSE)
    return RACE_UNDEFINED;

  if (race_list[i].level_adjustment > 0)
    return RACE_UNDEFINED;

  return i;

}
 */
