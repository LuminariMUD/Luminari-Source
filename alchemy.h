/* Alchemy subsystems for the alchemist class */
/* Gicker aka Stephen Squires, July 2019 */

/* alchemical discoveries */

#define ALC_DISC_NONE 0
#define ALC_DISC_ACID_BOMBS 1
#define ALC_DISC_BLINDING_BOMBS 2
#define ALC_DISC_BONESHARD_BOMBS 3
#define ALC_DISC_CELESTIAL_POISONS 4
#define ALC_DISC_CHAMELEON 5
#define ALC_DISC_COGNATOGEN 6
#define ALC_DISC_CONCUSSIVE_BOMBS 7
#define ALC_DISC_CONFUSION_BOMBS 8
#define ALC_DISC_DISPELLING_BOMBS 9
#define ALC_DISC_ELEMENTAL_MUTAGEN 10
#define ALC_DISC_ENHANCE_POTION 11
#define ALC_DISC_EXTEND_POTION 12
#define ALC_DISC_FAST_BOMBS 13
#define ALC_DISC_FIRE_BRAND 14
#define ALC_DISC_FORCE_BOMBS 15
#define ALC_DISC_FROST_BOMBS 16
#define ALC_DISC_GRAND_COGNATOGEN 17
#define ALC_DISC_GRAND_INSPIRING_COGNATOGEN 18
#define ALC_DISC_GRAND_MUTAGEN 19
#define ALC_DISC_GREATER_COGNATOGEN 20
#define ALC_DISC_GREATER_INSPIRING_COGNATOGEN 21
#define ALC_DISC_GREATER_MUTAGEN 22
#define ALC_DISC_HEALING_BOMBS 23
#define ALC_DISC_HEALING_TOUCH 24
#define ALC_DISC_HOLY_BOMBS 25
#define ALC_DISC_IMMOLATION_BOMBS 26
#define ALC_DISC_INFUSE_MUTAGEN 27
#define ALC_DISC_INFUSION 28
#define ALC_DISC_INSPIRING_COGNATOGEN 29
#define ALC_DISC_MALIGNANT_POISON 30
#define ALC_DISC_POISON_BOMBS 31
#define ALC_DISC_PRECISE_BOMBS 32
#define ALC_DISC_PRESERVE_ORGANS 33
#define ALC_DISC_PROFANE_BOMBS 34
#define ALC_DISC_PSYCHOKINETIC_TINCTURE 35
#define ALC_DISC_SHOCK_BOMBS 36
#define ALC_DISC_SPONTANEOUS_HEALING 37
#define ALC_DISC_STICKY_BOMBS 38
#define ALC_DISC_STINK_BOMBS 39
#define ALC_DISC_SUNLIGHT_BOMBS 40
#define ALC_DISC_TANGLEFOOT_BOMBS 41
#define ALC_DISC_VESTIGIAL_ARM 42
#define ALC_DISC_WINGS 43

// The value below is now recorded in structs.h
//#define NUM_ALC_DISCOVERIES				44

#define GR_ALC_DISC_NONE 0
#define GR_ALC_DISC_AWAKENED_INTELLECT 1
#define GR_ALC_DISC_FAST_HEALING 2
#define GR_ALC_DISC_POISON_TOUCH 3
#define GR_ALC_DISC_TRUE_MUTAGEN 4

//#define NUM_GR_ALC_DISCOVERIES 5

#define BOMB_NONE 0
#define BOMB_NORMAL 1
#define BOMB_ACID 2
#define BOMB_BLINDING 3
#define BOMB_BONESHARD 4
#define BOMB_CONCUSSIVE 5
#define BOMB_CONFUSION 6
#define BOMB_DISPELLING 7
#define BOMB_FIRE_BRAND 8
#define BOMB_FORCE 9
#define BOMB_FROST 10
#define BOMB_HEALING 11
#define BOMB_HOLY 12
#define BOMB_IMMOLATION 13
#define BOMB_POISON 14
#define BOMB_PROFANE 15
#define BOMB_SHOCK 16
#define BOMB_STINK 17
#define BOMB_SUNLIGHT 18
#define BOMB_TANGLEFOOT 19

#define NUM_BOMB_TYPES 20

#define KNOWS_DISCOVERY(ch, i)  (ch->player_specials->saved.discoveries[i])
#define GET_GRAND_DISCOVERY(ch) (ch->player_specials->saved.grand_discovery)
#define GET_BOMB(ch, i)         (ch->player_specials->saved.bombs[i])
#define GET_STICKY_BOMB(ch, i)  (ch->sticky_bomb[i])

ACMD_DECL(do_bombs);
ACMD_DECL(do_discoveries);
ACMD_DECL(do_grand_discoveries);
ACMD_DECL(do_swallow);
ACMD_DECL(do_curingtouch);
ACMD_DECL(do_poisontouch);
ACMD_DECL(do_psychokinetic);
void perform_bomb_effect(struct char_data *ch, struct char_data *victim, int bomb_type);
void list_bomb_types_known(struct char_data *ch);
int find_open_bomb_slot(struct char_data *ch);
int bomb_type_to_discovery(int bomb);
int discovery_to_bomb_type(int discovery);
int num_of_bombs_prepared(struct char_data *ch);
int num_of_bombs_preparable(struct char_data *ch);
void perform_bomb_direct_effect(struct char_data *ch, struct char_data *victim, int bomb_type);
void perform_bomb_direct_damage(struct char_data *ch, struct char_data *victim, int bomb_type);
void perform_bomb_splash_effect(struct char_data *ch, struct char_data *victim, int bomb_type);
void perform_bomb_splash_damage(struct char_data *ch, struct char_data *victim, int bomb_type);
void perform_bomb_direct_healing(struct char_data *ch, struct char_data *victim, int bomb_type);
void perform_bomb_self_effect(struct char_data *ch, struct char_data *victim, int bomb_type);
void perform_bomb_spell_effect(struct char_data *ch, struct char_data *victim, int bomb_type);
int can_learn_discovery(struct char_data *ch, int discovery);
void send_bomb_direct_message(struct char_data *ch, struct char_data *victim, int bomb_type);
void send_bomb_splash_message(struct char_data *ch, struct char_data *victim, int bomb_type);
int num_alchemical_discoveries_known(struct char_data *ch);
sbyte has_alchemist_discoveries_unchosen(struct char_data *ch);
sbyte has_alchemist_discoveries_unchosen_study(struct char_data *ch);
int list_alchemical_discoveries(struct char_data *ch);
sbyte bomb_is_friendly(int bomb);
void perform_mutagen(struct char_data *ch, char *arg2);
void perform_elemental_mutagen(struct char_data *ch, char *arg2);
void perform_cognatogen(struct char_data *ch, char *arg2);
void perform_inspiring_cognatogen(struct char_data *ch);
void clear_mutagen(struct char_data *ch);
void add_sticky_bomb_effect(struct char_data *ch, struct char_data *vict, int bomb_type);
bool display_discovery_info(struct char_data *ch, char *discoveryname);
bool display_grand_discovery_info(struct char_data *ch, char *discoveryname);
bool display_bomb_types(struct char_data *ch, char *keyword);
bool display_discovery_types(struct char_data *ch, char *keyword);
ACMDCHECK(can_swallow);

extern const char *bomb_damage_messages[NUM_BOMB_TYPES][3];
extern const char *bomb_requisites[NUM_BOMB_TYPES];
extern const char *bomb_descriptions[NUM_BOMB_TYPES];
extern const char *bomb_types[NUM_BOMB_TYPES];
extern const char *grand_alchemical_discovery_names[NUM_GR_ALC_DISCOVERIES];
extern const char *alchemical_discovery_names[NUM_ALC_DISCOVERIES];
extern const char *alchemical_discovery_descriptions[NUM_ALC_DISCOVERIES];
extern const char *discovery_requisites[NUM_ALC_DISCOVERIES];
