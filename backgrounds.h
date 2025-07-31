
#define BACKGROUND_NONE         0
#define BACKGROUND_ACOLYTE      1
#define BACKGROUND_CHARLATAN    2
#define BACKGROUND_CRIMINAL     3
#define BACKGROUND_ENTERTAINER  4
#define BACKGROUND_FOLK_HERO    5
#define BACKGROUND_GLADIATOR    6
#define BACKGROUND_TRADER       7
#define BACKGROUND_HERMIT       8
#define BACKGROUND_SQUIRE       9
#define BACKGROUND_NOBLE        10
#define BACKGROUND_OUTLANDER    11
#define BACKGROUND_PIRATE       12
#define BACKGROUND_SAGE         13
#define BACKGROUND_SAILOR       14
#define BACKGROUND_SOLDIER      15
#define BACKGROUND_URCHIN       16

#define NUM_BACKGROUNDS         17

#define FORAGE_FOOD_ITEM_VNUM   13820
#define RETAINER_MOB_VNUM       13824

void assign_backgrounds(void);
void sort_backgrounds(void);
bool has_acolyte_in_group(struct char_data *ch);
void show_background_help(struct char_data *ch, int background);
ACMD_DECL(do_swindle);
ACMD_DECL(do_entertain);
ACMD_DECL(do_forgeas);
ACMD_DECL(do_relay);
ACMD_DECL(do_tribute);
ACMD_DECL(do_forage);
ACMD_DECL(do_extort);
ACMD_DECL(do_retainer);
ACMD_DECL(do_shortcut);

struct background_data
{
    const char *name;
    const char *desc;
    int skills[2];
    int feat;
};

extern int background_sort_info[NUM_BACKGROUNDS];
extern struct background_data background_list[NUM_BACKGROUNDS];
extern int backgrounds_listed_alphabetically[NUM_BACKGROUNDS];