/* psionic power types */
#define PSIONIC_POWER_TYPE_NONE 0
#define CLAIRSENTIENCE          1
#define METACREATIVITY          2
#define PSYCHOKINESIS           3
#define PSYCHOMETABOLISM        4
#define PSYCHOPORTATION         5
#define TELEPATHY               6

#define NUM_PSIONIC_POWER_TYPES 7

struct psionic_power_data
{
  short int spellnum;
  short int psp_cost;
  bool can_augment;
  int augment_amount;
  int max_augment;
  short int action_type;
  short int power_level;
  short int power_type;
};

void assign_psionic_powers(void);
int max_augment_psp_allowed(struct char_data *ch, int spellnum);
int adjust_augment_psp_for_spell(struct char_data *ch, int spellnum);
int get_augment_casting_time_adjustment(struct char_data *ch);
extern struct psionic_power_data psionic_powers[];