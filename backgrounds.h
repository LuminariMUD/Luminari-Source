
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

void assign_backgrounds(void);
void sort_backgrounds(void);
bool has_acolyte_in_group(struct char_data *ch);
ACMD_DECL(do_swindle);

struct background_data
{
    const char *name;
    const char *desc;
    int skills[2];
    int feat;
};