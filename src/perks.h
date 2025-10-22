/**
 * @file perks.h
 * Header file for the Perks System
 */

#ifndef _PERKS_H_
#define _PERKS_H_

/* Function prototypes */

/* Initialization */
void init_perks(void);
int count_defined_perks(void);

/* Perk definition functions */
void define_fighter_perks(void);
void define_wizard_perks(void);
void define_cleric_perks(void);
void define_rogue_perks(void);
void define_ranger_perks(void);
void define_barbarian_perks(void);

/* Lookup functions */
struct perk_data *get_perk_by_id(int perk_id);
int get_class_perks(int class_id, int *perk_ids, int max_perks);
bool perk_exists(int perk_id);
const char *get_perk_name(int perk_id);
const char *get_perk_description(int perk_id);

/* Stage progression functions (step 3) */
void init_stage_data(struct char_data *ch);
void update_stage_data(struct char_data *ch);
int calculate_stage_xp_needed(struct char_data *ch);
bool check_stage_advancement(struct char_data *ch, int *perk_points_awarded);
void award_stage_perk_points(struct char_data *ch, int class_id);

/* Perk point tracking functions (step 4) */
int get_perk_points(struct char_data *ch, int class_id);
bool spend_perk_points(struct char_data *ch, int class_id, int amount);
void add_perk_points(struct char_data *ch, int class_id, int amount);
int get_total_perk_points(struct char_data *ch);
void display_perk_points(struct char_data *ch);

/* Perk purchase/management functions (step 5) */
bool has_perk(struct char_data *ch, int perk_id);
int get_perk_rank(struct char_data *ch, int perk_id, int class_id);
int get_total_perk_ranks(struct char_data *ch, int perk_id);
bool can_purchase_perk(struct char_data *ch, int perk_id, int class_id, char *error_msg, size_t error_len);
bool purchase_perk(struct char_data *ch, int perk_id, int class_id);
struct char_perk_data *find_char_perk(struct char_data *ch, int perk_id, int class_id);
void add_char_perk(struct char_data *ch, int perk_id, int class_id);
void remove_char_perk(struct char_data *ch, int perk_id, int class_id);
int count_char_perks(struct char_data *ch);

#endif /* _PERKS_H_ */
