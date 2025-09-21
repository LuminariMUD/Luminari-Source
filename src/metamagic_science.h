/* 
 * metamagic_science.h
 * Header file for Metamagic Science feat functionality
 */

#ifndef _METAMAGIC_SCIENCE_H_
#define _METAMAGIC_SCIENCE_H_

/* Function prototypes */

/* Parse metamagic arguments for consumable items
 * Returns parsed metamagic flags, or -1 on error */
int parse_metamagic_for_consumables(struct char_data *ch, char **argument, int spell_num, int item_type);

/* Calculate additional charges needed for metamagic on wands/staves */
int calculate_metamagic_charge_cost(int metamagic, int base_spell_level);

/* Calculate Use Magic Device DC for metamagic scrolls and potions */
int calculate_metamagic_scroll_dc(int base_spell_level, int metamagic);

/* Get metamagic description string for display */
void get_metamagic_description(int metamagic, char *buf, size_t buf_size);

#endif /* _METAMAGIC_SCIENCE_H_ */
