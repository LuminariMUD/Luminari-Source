
#ifndef _EVOLUTIONS_H_
#define _EVOLUTIONS_H_

#define EVOLUTION_REQ_TYPE_NONE   0 // default, means no requirements
#define EVOLUTION_REQ_TYPE_ALL    1 // requires all of the evolutions listed
#define EVOLUTION_REQ_TYPE_ANY    2 // requires any of the evolutions listed
#define EVOLUTION_REQ_TYPE_UNIQUE 3 // can not be taken if they already have any of the evolutions listed

struct evolution_info 
{
    const char *name;
    const char *desc;
    int evolution_points;
    bool stacks;
    int stack_level;
    int evolution_requirements[6];
    int requirement_type;
    bool pc_avail;
};

// Summoner Eidolon Evolutions

#define EVOLUTION_NONE                      0
#define EVOLUTION_BITE                      1
#define EVOLUTION_BLEED                     2
#define EVOLUTION_CLAWS                     3
#define EVOLUTION_GILLS                     4
#define EVOLUTION_HOOVES                    5
#define EVOLUTION_IMPROVED_DAMAGE           6
#define EVOLUTION_IMPROVED_NATURAL_ARMOR    7
#define EVOLUTION_MAGIC_ATTACKS             8
#define EVOLUTION_MOUNT                     9
#define EVOLUTION_PINCERS                   10
#define EVOLUTION_PUSH                      11
#define EVOLUTION_REACH                     12
#define EVOLUTION_FIRE_RESIST               13
#define EVOLUTION_COLD_RESIST               14
#define EVOLUTION_ELECTRIC_RESIST           15
#define EVOLUTION_ACID_RESIST               16
#define EVOLUTION_SONIC_RESIST              17
#define EVOLUTION_SCENT                     18
#define EVOLUTION_SKILLED                   19
#define EVOLUTION_STING                     20
#define EVOLUTION_SWIM                      21
#define EVOLUTION_TAIL_SLAP                 22
#define EVOLUTION_TENTACLE                  23
#define EVOLUTION_WING_BUFFET               24
#define EVOLUTION_STR_INCREASE              25
#define EVOLUTION_DEX_INCREASE              26
#define EVOLUTION_CON_INCREASE              27
#define EVOLUTION_INT_INCREASE              28
#define EVOLUTION_WIS_INCREASE              29
#define EVOLUTION_CHA_INCREASE              30
#define EVOLUTION_THRASH_EVIL                31
#define EVOLUTION_THRASH_GOOD                32
#define EVOLUTION_CONSTRICT                 33
#define EVOLUTION_FIRE_ATTACK               34
#define EVOLUTION_COLD_ATTACK               35
#define EVOLUTION_ELECTRIC_ATTACK           36
#define EVOLUTION_ACID_ATTACK               37
#define EVOLUTION_SONIC_ATTACK              38
#define EVOLUTION_FLIGHT                    39
#define EVOLUTION_GORE                      40
#define EVOLUTION_MINOR_MAGIC               41
#define EVOLUTION_POISON                    42
#define EVOLUTION_RAKE                      43
#define EVOLUTION_REND                      44
#define EVOLUTION_RIDER_BOND                45
#define EVOLUTION_SHADOW_BLEND              46
#define EVOLUTION_SHADOW_FORM               47
#define EVOLUTION_SICKENING                 48
#define EVOLUTION_TRAMPLE                   49
#define EVOLUTION_TRIP                      50
#define EVOLUTION_UNDEAD_APPEARANCE         51
#define EVOLUTION_BLINDSIGHT                52
#define EVOLUTION_CELESTIAL_APPEARANCE      53
#define EVOLUTION_DAMAGE_REDUCTION          54
#define EVOLUTION_FIENDISH_APPEARANCE       55
#define EVOLUTION_FRIGHTFUL_PRESENCE        56
#define EVOLUTION_MAJOR_MAGIC               57
#define EVOLUTION_SACRIFICE                 58
#define EVOLUTION_WEB                       59
#define EVOLUTION_ACID_BREATH               60
#define EVOLUTION_FIRE_BREATH               61
#define EVOLUTION_COLD_BREATH               62
#define EVOLUTION_ELECTRIC_BREATH           63
#define EVOLUTION_FAST_HEALING              64
#define EVOLUTION_INCORPOREAL_FORM          65
#define EVOLUTION_LARGE                     66
#define EVOLUTION_SPELL_RESISTANCE          67
#define EVOLUTION_ULTIMATE_MAGIC            68
#define EVOLUTION_BASIC_MAGIC               69
#define EVOLUTION_KEEN_SCENT                70
#define EVOLUTION_HUGE                      71
#define EVOLUTION_PENETRATE_DR              72

// This is defined in structs.h
//#define NUM_EVOLUTIONS XX

extern struct evolution_info evolution_list[NUM_EVOLUTIONS];
bool qualifies_for_evolution(struct char_data *ch, int evolution);
bool is_evolution_attack(int attack_type);
int determine_evolution_attack_damage_dice(struct char_data *ch, int attack_type);
void perform_evolution_attack(struct char_data *ch, int mode, int phase, int attack_type, int dam_type);
void apply_evolution_bleed(struct char_data *ch);
void apply_evolution_poison(struct char_data *ch, struct char_data *vict);
void process_evolution_elemental_damage(struct char_data *ch, struct char_data *victim);
void process_evolution_thrash_alignment_damage(struct char_data *ch, struct char_data *victim);
void assign_eidolon_evolutions(struct char_data *ch, struct char_data *mob, bool from_db);
void process_evolution_breath_damage(struct char_data *ch);
int num_evo_breaths(struct char_data *ch);
bool is_eidolon_in_room(struct char_data *ch);
struct char_data *get_eidolon_in_room(struct char_data *ch);
ACMD_DECL(do_eidolon);
int get_shield_ally_bonus(struct char_data *ch);
void merge_eidolon_evolutions(struct char_data *ch);
void display_evolution_requirements(struct char_data *ch, int evo);
bool display_evolution_info(struct char_data *ch, const char *evoname);
int find_evolution_num(const char *name);
void assign_evolutions(void);
bool display_evolution_info(struct char_data *ch, const char *evoname);
bool study_qualifies_for_evolution(struct char_data *ch, int evolution, bool is_pc);
bool is_eidolon_base_form_evolution(int form, int evo);
void study_assign_eidolon_base_form(struct char_data *ch, int form);
bool study_evolution_already_taken_or_maxxed(struct char_data *ch, int evolution);
int study_num_aspects_chosen(struct descriptor_data *d);
bool study_has_aspects_unchosen(struct descriptor_data *d);
int get_evolution_attack_w_type(int attack_type);

#endif