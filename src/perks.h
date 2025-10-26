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
/* Stage-based progression functions */
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
bool has_perk_active(struct char_data *ch, int perk_id);
bool is_perk_toggled_on(struct char_data *ch, int perk_id);
void set_perk_toggle(struct char_data *ch, int perk_id, bool state);
int get_perk_rank(struct char_data *ch, int perk_id, int class_id);
int get_total_perk_ranks(struct char_data *ch, int perk_id);
bool can_purchase_perk(struct char_data *ch, int perk_id, int class_id, char *error_msg, size_t error_len);
bool purchase_perk(struct char_data *ch, int perk_id, int class_id);
struct char_perk_data *find_char_perk(struct char_data *ch, int perk_id, int class_id);
void add_char_perk(struct char_data *ch, int perk_id, int class_id);
int count_char_perks(struct char_data *ch);

/* Perk effects functions (step 6) */
void apply_all_perk_effects(struct char_data *ch);
int get_perk_bonus(struct char_data *ch, int effect_type, int effect_modifier);
int get_perk_hp_bonus(struct char_data *ch);
int get_perk_spell_points_bonus(struct char_data *ch);
int get_perk_ac_bonus(struct char_data *ch);
int get_perk_save_bonus(struct char_data *ch, int save_type);
int get_perk_skill_bonus(struct char_data *ch, int skill_num);
int get_perk_weapon_damage_bonus(struct char_data *ch, struct obj_data *wielded);
int get_perk_weapon_tohit_bonus(struct char_data *ch, struct obj_data *wielded);

/* Rogue-specific perk bonus functions */
int get_perk_sneak_attack_dice(struct char_data *ch);
int get_perk_critical_confirmation_bonus(struct char_data *ch);
int get_perk_ranged_sneak_attack_bonus(struct char_data *ch);
int get_perk_assassinate_bonus(struct char_data *ch);
int get_perk_aoo_bonus(struct char_data *ch);
int get_perk_critical_precision_damage(struct char_data *ch);
bool has_master_assassin(struct char_data *ch);
int get_master_assassin_crit_bonus(struct char_data *ch);

/* Perfect Kill perk functions */
bool can_use_perfect_kill(struct char_data *ch);
void use_perfect_kill(struct char_data *ch);
void update_perfect_kill_combat_end(struct char_data *ch);

/* Master Thief perk functions */
int get_perk_skill_mastery_bonus(struct char_data *ch);
int get_perk_trapfinding_bonus(struct char_data *ch);
int get_perk_fast_hands_bonus(struct char_data *ch);
int get_perk_trap_sense_bonus(struct char_data *ch);

/* Tier 3 and 4 Master Thief perk functions */
bool has_shadow_step(struct char_data *ch);
int get_shadow_step_bonus(struct char_data *ch);
bool can_take_10_on_rogue_skills(struct char_data *ch);
bool has_legendary_reflexes(struct char_data *ch);
int get_legendary_reflexes_save_bonus(struct char_data *ch);

/* Shadow Scout tree perk functions */
int get_perk_stealth_mastery_bonus(struct char_data *ch);
int get_perk_fleet_of_foot_bonus(struct char_data *ch);
int get_perk_awareness_bonus(struct char_data *ch);
bool has_light_step(struct char_data *ch);
bool has_hide_in_plain_sight(struct char_data *ch);
bool has_shadow_step_teleport(struct char_data *ch);
int get_shadow_step_range(struct char_data *ch);
bool has_uncanny_dodge(struct char_data *ch);
int get_perk_acrobatics_bonus(struct char_data *ch);
int get_perk_acrobatics_ac_bonus(struct char_data *ch);
bool has_awareness_3(struct char_data *ch);
int get_perk_blindsense_range(struct char_data *ch);
bool has_uncanny_dodge_2(struct char_data *ch);
int get_uncanny_dodge_aoo_ac_bonus(struct char_data *ch);
bool can_disengage_free_action(struct char_data *ch);
bool has_vanish(struct char_data *ch);
bool has_shadow_master(struct char_data *ch);
bool has_ghost(struct char_data *ch);

/* Cleric Divine Healer tree perk functions */
int get_cleric_healing_power_bonus(struct char_data *ch);
int get_cleric_radiant_servant_bonus(struct char_data *ch);
bool has_efficient_healing(struct char_data *ch);
int get_preserve_life_bonus(struct char_data *ch, struct char_data *target);
int get_mass_healing_focus_targets(struct char_data *ch);
bool is_healing_empowered(struct char_data *ch);
int get_empowered_healing_multiplier(struct char_data *ch);
bool has_channel_energy_heal(struct char_data *ch);
int get_channel_energy_dice(struct char_data *ch);
bool has_healing_aura(struct char_data *ch);
int get_healing_aura_bonus(struct char_data *ch);
int get_healing_aura_range(struct char_data *ch);
bool has_restorative_touch(struct char_data *ch);
bool has_divine_radiance(struct char_data *ch);
bool has_beacon_of_hope(struct char_data *ch);

/* Cleric Battle Cleric tree perk functions */
int get_cleric_divine_favor_bonus(struct char_data *ch);
int get_cleric_holy_weapon_bonus(struct char_data *ch);
int get_cleric_armor_of_faith_bonus(struct char_data *ch);
bool has_battle_blessing(struct char_data *ch);
int get_cleric_smite_evil_dice(struct char_data *ch);
int get_cleric_smite_evil_uses(struct char_data *ch);
int get_cleric_divine_power_bonus(struct char_data *ch);
bool has_channel_energy_harm(struct char_data *ch);
bool has_spiritual_weapon(struct char_data *ch);

/* Battle Cleric Tier 3 & 4 helper functions */
int get_cleric_armor_of_faith_save_bonus(struct char_data *ch);
bool has_righteous_fury(struct char_data *ch);
bool has_divine_wrath(struct char_data *ch);
int get_cleric_total_holy_weapon_bonus(struct char_data *ch);
int get_cleric_total_divine_favor_bonus(struct char_data *ch);

/* Cleric Domain Master tree perk functions */
bool is_divine_spellcasting_class(int class_num);
int get_cleric_domain_focus_bonus(struct char_data *ch);
int get_cleric_divine_spell_power_bonus(struct char_data *ch);
int get_cleric_bonus_domain_spells(struct char_data *ch);
int get_cleric_bonus_spell_slots(struct char_data *ch);
int get_cleric_turn_undead_enhancement_bonus(struct char_data *ch);
int get_cleric_extended_domain_bonus(struct char_data *ch);
int get_cleric_divine_metamagic_reduction(struct char_data *ch);
bool has_destroy_undead(struct char_data *ch);
int get_cleric_greater_turning_bonus(struct char_data *ch);
int get_cleric_domain_mastery_bonus(struct char_data *ch);
int get_cleric_master_of_undead_dc_bonus(struct char_data *ch);
bool has_control_undead(struct char_data *ch);
int get_destroy_undead_threshold(struct char_data *ch);

/* Wizard Evoker tree perk functions */
int get_wizard_spell_power_bonus(struct char_data *ch);
int get_master_of_elements_override(struct char_data *ch, int dam_type);
int get_wizard_elemental_damage_bonus(struct char_data *ch, int dam_type);
int get_wizard_elemental_dc_bonus(struct char_data *ch, int dam_type);
int get_wizard_spell_penetration_bonus(struct char_data *ch);
bool has_wizard_spell_critical(struct char_data *ch);
int get_wizard_spell_critical_chance(struct char_data *ch);
int get_wizard_spell_critical_multiplier(struct char_data *ch);
bool can_use_maximize_spell_perk(struct char_data *ch);
void use_maximize_spell_perk(struct char_data *ch);
bool can_use_empower_spell_perk(struct char_data *ch);
void use_empower_spell_perk(struct char_data *ch);
int get_arcane_annihilation_bonus_dice(struct char_data *ch);
int get_arcane_annihilation_dc_bonus(struct char_data *ch);

/* Perk command functions (step 7) */
ACMD_DECL(do_perk);
ACMD_DECL(do_myperks);

/* Perk refund/reset functions (step 10) */
bool remove_char_perk(struct char_data *ch, int perk_id, int class_id);  /* Returns TRUE if removed */
void remove_class_perks(struct char_data *ch, int class_id);              /* Remove all perks for a class */
void remove_all_perks(struct char_data *ch);                               /* Remove all perks (for respec) */
void reset_all_perk_points(struct char_data *ch);                          /* Reset all perk points to 0 */

#endif /* _PERKS_H_ */
