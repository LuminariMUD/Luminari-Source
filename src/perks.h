/**
 * @file perks.h
 * Header file for the Perks System
 */

#ifndef _PERKS_H_
#define _PERKS_H_

/* External declarations */
extern const char *perk_category_names[];

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
void define_monk_perks(void);
void define_druid_perks(void);
void define_paladin_perks(void);

/* Lookup functions */
struct perk_data *get_perk_by_id(int perk_id);
int get_class_perks(int class_id, int *perk_ids, int max_perks);
bool perk_exists(int perk_id);
const char *get_perk_name(int perk_id);
const char *get_perk_description(int perk_id);
const char *get_perk_category_name(int perk_category);

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

/* Ranger-specific perk bonus functions */
int get_ranger_ranged_tohit_bonus(struct char_data *ch, struct obj_data *wielded);
int get_ranger_ranged_damage_bonus(struct char_data *ch, struct obj_data *wielded);
int get_ranger_dr_penetration(struct char_data *ch);
int get_ranger_attack_speed_bonus(struct char_data *ch);
int get_ranger_quick_draw_proc_chance(struct char_data *ch);

/* Ranger Beast Master perk functions */
int get_ranger_companion_hp_bonus(struct char_data *ch);
int get_ranger_companion_ac_bonus(struct char_data *ch);
int get_ranger_companion_tohit_bonus(struct char_data *ch);
int get_ranger_companion_save_bonus(struct char_data *ch);
bool ranger_companion_immune_fear(struct char_data *ch);
bool ranger_companion_immune_mind(struct char_data *ch);
int get_pack_tactics_bonus(struct char_data *ch, struct char_data *master, struct char_data *victim);
int get_coordinated_attack_damage(struct char_data *ch);
int get_primal_avatar_damage(struct char_data *ch);
int get_natures_remedy_bonus(struct char_data *ch);
bool has_primal_vigor(struct char_data *ch);
int get_ranger_conjuration_dc_bonus(struct char_data *ch);
int get_greater_summons_hp_bonus(struct char_data *ch);
int get_greater_summons_damage(struct char_data *ch);

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
float get_master_enchanter_duration_multiplier(struct char_data *ch);
float get_master_transmuter_duration_multiplier(struct char_data *ch);
float get_archmage_control_duration_multiplier(struct char_data *ch);
int get_archmage_control_dc_bonus(struct char_data *ch);
int get_master_enchanter_dc_bonus(struct char_data *ch);
int get_master_transmuter_dc_bonus(struct char_data *ch);
int get_master_illusionist_dc_bonus(struct char_data *ch);
int get_spell_mastery_dc_bonus(struct char_data *ch);

/* Wizard Versatile Caster tree perk functions */
int get_combat_casting_concentration_bonus(struct char_data *ch);
bool can_use_spell_recall(struct char_data *ch);
int get_metamagic_master_reduction(struct char_data *ch);
int get_arcane_knowledge_spellcraft_bonus(struct char_data *ch);
bool check_spell_slot_preservation(struct char_data *ch);

/* Wizard Controller tree perk functions */
int get_enchantment_spell_dc_bonus(struct char_data *ch);
int get_extend_spell_bonus(struct char_data *ch, int spellnum);
bool can_use_persistent_spell_perk(struct char_data *ch);
void use_persistent_spell_perk(struct char_data *ch);
bool is_persistent_spell_active(struct char_data *ch);
void clear_persistent_spell_active(struct char_data *ch);
bool can_use_split_enchantment_perk(struct char_data *ch);
void use_split_enchantment_perk(struct char_data *ch);

/* Defensive Casting perk functions */
void activate_defensive_casting(struct char_data *ch);
bool has_defensive_casting_active(struct char_data *ch);
int get_defensive_casting_ac_bonus(struct char_data *ch);

/* Versatile Caster Tier 3-4 perk functions */
void activate_spell_shield(struct char_data *ch);
bool has_spell_shield_active(struct char_data *ch);
int get_spell_shield_ac_bonus(struct char_data *ch);
int get_spell_shield_dr(struct char_data *ch);
int get_metamagic_circle_reduction(struct char_data *ch);
bool can_use_metamagic_reduction(struct char_data *ch);
void use_metamagic_reduction(struct char_data *ch);
int get_arcane_supremacy_dc_bonus(struct char_data *ch);
int get_arcane_supremacy_caster_level_bonus(struct char_data *ch);
int get_arcane_supremacy_damage_bonus(struct char_data *ch);

/* Monk perk helper functions */
int get_monk_unarmed_damage_bonus(struct char_data *ch);
int get_monk_dr_bypass(struct char_data *ch);
int get_monk_reflex_save_bonus(struct char_data *ch);
int get_monk_trip_bonus(struct char_data *ch);
bool can_monk_trip_during_flurry(struct char_data *ch);
int get_monk_hp_regen_bonus(struct char_data *ch);
int get_monk_stunning_fist_bonus_uses(struct char_data *ch);
int get_monk_stunning_fist_dc_bonus(struct char_data *ch);
int get_monk_stunning_fist_duration_bonus(struct char_data *ch);
int get_monk_unarmed_crit_range(struct char_data *ch);
bool has_monk_tiger_claw_bleed(struct char_data *ch);
bool can_use_monk_weapons_with_abilities(struct char_data *ch);
int get_monk_weapon_ac_bonus(struct char_data *ch, struct obj_data *weapon);
int get_monk_weapon_attack_bonus(struct char_data *ch, struct obj_data *weapon);
int get_monk_weapon_damage_bonus(struct char_data *ch, struct obj_data *weapon);
int get_monk_flurry_penalty_reduction(struct char_data *ch);
bool check_monk_extra_flurry_attack(struct char_data *ch);
int get_monk_power_strike_penalty(struct char_data *ch);
int get_monk_legendary_fist_damage(struct char_data *ch);
bool has_monk_legendary_fist(struct char_data *ch);
bool has_monk_crushing_blow(struct char_data *ch);
bool has_monk_shattering_strike(struct char_data *ch);

/* Shadow monk perk functions */
int get_monk_shadow_step_bonus(struct char_data *ch);
int get_monk_improved_hide_bonus(struct char_data *ch);
int get_monk_acrobatic_defense_ac(struct char_data *ch);
int get_monk_acrobatic_defense_skill(struct char_data *ch);
int get_monk_deadly_precision_dice(struct char_data *ch);
bool has_monk_vanishing_technique(struct char_data *ch);
bool has_monk_shadow_clone(struct char_data *ch);
bool has_monk_smoke_bomb(struct char_data *ch);
bool has_monk_pressure_point_strike(struct char_data *ch);
bool has_monk_shadow_step_iii(struct char_data *ch);
int get_monk_deadly_precision_iii_dice(struct char_data *ch);
int get_monk_deadly_precision_iii_crit_dice(struct char_data *ch);
bool has_monk_assassinate(struct char_data *ch);
bool has_monk_shadow_fade(struct char_data *ch);
bool has_monk_blinding_speed(struct char_data *ch);
bool has_monk_shadow_master(struct char_data *ch);
bool has_monk_void_strike(struct char_data *ch);

/* Way of the Four Elements perks */
int get_monk_elemental_attunement_i_rank(struct char_data *ch);
bool has_monk_fangs_of_fire_snake(struct char_data *ch);
bool has_monk_water_whip(struct char_data *ch);
bool has_monk_gong_of_summit(struct char_data *ch);
int get_monk_fist_of_unbroken_air_rank(struct char_data *ch);
int get_monk_elemental_resistance_i_rank(struct char_data *ch);
int get_monk_elemental_attunement_ii_rank(struct char_data *ch);
int get_monk_elemental_attunement_iii_rank(struct char_data *ch);
bool has_monk_shape_flowing_river(struct char_data *ch);
bool has_monk_sweeping_cinder_strike(struct char_data *ch);
bool has_monk_rush_of_gale_spirits(struct char_data *ch);
bool has_monk_clench_north_wind(struct char_data *ch);
bool has_monk_elemental_resistance_ii(struct char_data *ch);
bool has_monk_mist_stance(struct char_data *ch);
bool has_monk_swarming_ice_rabbit(struct char_data *ch);
/* Tier 3 */
bool has_monk_flames_of_phoenix(struct char_data *ch);
bool has_monk_wave_of_rolling_earth(struct char_data *ch);
bool has_monk_ride_the_wind(struct char_data *ch);
bool has_monk_eternal_mountain_defense(struct char_data *ch);
bool has_monk_fist_of_four_thunders(struct char_data *ch);
bool has_monk_river_of_hungry_flame(struct char_data *ch);
/* Tier 4 */
bool has_monk_breath_of_winter(struct char_data *ch);
bool has_monk_elemental_embodiment(struct char_data *ch);
bool has_monk_avatar_of_elements(struct char_data *ch);

/* Druid perk helper functions (Nature's Warrior tree) */
int get_druid_wild_shape_attack_bonus(struct char_data *ch);
int get_druid_wild_shape_damage_bonus(struct char_data *ch);
int get_druid_natural_armor_bonus(struct char_data *ch);
int get_druid_wild_shape_hp_bonus(struct char_data *ch);
int get_druid_natural_weapons_damage_dice(struct char_data *ch);
bool has_druid_natural_weapons_improved_crit(struct char_data *ch);
bool is_druid_in_elemental_form(struct char_data *ch);
int get_druid_elemental_attack_bonus(struct char_data *ch);
int get_druid_elemental_damage_bonus(struct char_data *ch);
int get_druid_elemental_armor_bonus(struct char_data *ch);
int get_druid_elemental_hp_bonus(struct char_data *ch);
bool has_druid_primal_avatar(struct char_data *ch);
bool has_druid_natural_fury(struct char_data *ch);

/* Season's Herald druid perk helpers */
int get_druid_spell_power_bonus(struct char_data *ch);
int get_druid_spell_dc_bonus(struct char_data *ch);
int get_druid_elemental_damage_dice(struct char_data *ch);
bool check_druid_spell_critical(struct char_data *ch);
float get_druid_spell_critical_multiplier(struct char_data *ch);
int get_druid_bonus_spell_slots(struct char_data *ch);
bool has_druid_force_of_nature(struct char_data *ch);
bool has_druid_storm_caller(struct char_data *ch);
bool has_druid_elemental_mastery(struct char_data *ch);

/* Berserker/Barbarian perk helper functions - Tier 1 & 2 */
int get_berserker_power_attack_bonus(struct char_data *ch);
int get_berserker_rage_damage_bonus(struct char_data *ch);
int get_berserker_critical_bonus(struct char_data *ch);
bool has_berserker_cleaving_strikes(struct char_data *ch);
int get_berserker_cleave_bonus(struct char_data *ch);
bool has_berserker_blood_frenzy(struct char_data *ch);
int get_berserker_devastating_critical_dice(struct char_data *ch);

/* Berserker/Barbarian perk helper functions - Tier 3 */
int get_berserker_power_attack_mastery_3_bonus(struct char_data *ch);
bool has_berserker_overwhelming_force(struct char_data *ch);
int get_berserker_crimson_rage_bonus(struct char_data *ch);
bool has_berserker_carnage(struct char_data *ch);

/* Berserker/Barbarian perk helper functions - Tier 4 */
bool has_berserker_frenzied_berserker(struct char_data *ch);
bool has_berserker_relentless_assault(struct char_data *ch);
bool has_berserker_death_from_above(struct char_data *ch);

/* Berserker/Barbarian Occult Slayer tree helper functions */
int get_berserker_thick_skin_bonus(struct char_data *ch);
int get_berserker_damage_reduction(struct char_data *ch);
int get_berserker_elemental_resistance(struct char_data *ch);
bool has_berserker_hardy(struct char_data *ch);
int get_berserker_savage_defiance_dr(struct char_data *ch);
int get_berserker_damage_reduction_3(struct char_data *ch);
int get_berserker_spell_resistance(struct char_data *ch);
bool has_berserker_pain_tolerance(struct char_data *ch);
bool has_berserker_deathless_frenzy(struct char_data *ch);
int get_berserker_unstoppable_dr(struct char_data *ch);
bool has_berserker_indomitable_will(struct char_data *ch);
bool has_berserker_raging_defender(struct char_data *ch);

/* Berserker/Barbarian Primal Warrior tree helper functions - Tier 1 & 2 */
int get_berserker_fleet_of_foot_bonus(struct char_data *ch);
int get_berserker_intimidating_presence_bonus(struct char_data *ch);
int get_berserker_intimidating_presence_morale_penalty(struct char_data *ch);
bool has_berserker_mighty_leap(struct char_data *ch);
int get_berserker_mighty_leap_bonus(struct char_data *ch);
int get_berserker_thick_headed_bonus(struct char_data *ch);
bool has_berserker_sprint(struct char_data *ch);
bool has_berserker_intimidating_presence_2(struct char_data *ch);
int get_berserker_crippling_blow_chance(struct char_data *ch);

/* Berserker/Barbarian Primal Warrior tree helper functions - Tier 3 & 4 */
bool has_berserker_reckless_abandon(struct char_data *ch);
bool has_berserker_blinding_rage(struct char_data *ch);
bool has_berserker_stunning_blow(struct char_data *ch);
bool has_berserker_uncanny_dodge_mastery(struct char_data *ch);
int get_berserker_uncanny_dodge_perception_bonus(struct char_data *ch);
int get_berserker_uncanny_dodge_ac_bonus(struct char_data *ch);
bool has_berserker_savage_charge(struct char_data *ch);
bool has_berserker_war_cry(struct char_data *ch);
bool has_berserker_earthshaker(struct char_data *ch);

/* Paladin perk helper functions - Knight of the Chalice */
int get_paladin_holy_weapon_damage_bonus(struct char_data *ch, struct char_data *victim);
int get_paladin_sacred_defender_ac_bonus(struct char_data *ch);
int get_paladin_improved_smite_dice(struct char_data *ch);
bool has_paladin_faithful_strike(struct char_data *ch);
bool has_paladin_holy_blade(struct char_data *ch);
bool has_paladin_divine_might(struct char_data *ch);
bool has_paladin_exorcism_of_the_slain(struct char_data *ch);
bool has_paladin_holy_sword(struct char_data *ch);
bool has_paladin_zealous_smite(struct char_data *ch);
bool has_paladin_blinding_smite(struct char_data *ch);
bool has_paladin_overwhelming_smite(struct char_data *ch);
bool has_paladin_sacred_vengeance(struct char_data *ch);

/* Paladin perk helper functions - Sacred Defender */
int get_paladin_extra_lay_on_hands(struct char_data *ch);
int get_paladin_shield_of_faith_ac_bonus(struct char_data *ch);
int get_paladin_bulwark_saves_bonus(struct char_data *ch);
int get_paladin_healing_hands_bonus(struct char_data *ch);
bool has_paladin_defensive_strike(struct char_data *ch);
bool has_paladin_shield_guardian(struct char_data *ch);
bool has_paladin_aura_of_protection(struct char_data *ch);
int get_paladin_sanctuary_reduction(struct char_data *ch);
bool has_paladin_merciful_touch(struct char_data *ch);
bool has_paladin_bastion_of_defense(struct char_data *ch);
bool has_paladin_aura_of_life(struct char_data *ch);
bool has_paladin_cleansing_touch(struct char_data *ch);
bool has_paladin_divine_sacrifice(struct char_data *ch);

/* Divine Champion Tree Helper Functions */
int get_paladin_spell_focus_bonus(struct char_data *ch);
int get_paladin_turn_undead_hd_bonus(struct char_data *ch);
int get_paladin_turn_undead_damage_bonus(struct char_data *ch);
int get_paladin_divine_grace_bonus(struct char_data *ch);
bool has_paladin_radiant_aura(struct char_data *ch);
bool has_paladin_quickened_blessing(struct char_data *ch);
int get_paladin_channel_energy_dice(struct char_data *ch);
int get_paladin_channel_energy_uses(struct char_data *ch);
bool is_quickened_blessing_spell(int spellnum);

/* Divine Champion Tier 3 helper functions */
bool has_paladin_spell_penetration(struct char_data *ch);
bool has_paladin_destroy_undead(struct char_data *ch);
int get_paladin_channel_energy_2_dice(struct char_data *ch);
int get_paladin_channel_energy_2_uses(struct char_data *ch);
bool has_paladin_aura_of_courage_mastery(struct char_data *ch);

/* Divine Champion Tier 4 helper functions */
bool has_paladin_mass_cure_wounds(struct char_data *ch);
bool has_paladin_holy_avenger(struct char_data *ch);
bool has_paladin_beacon_of_hope(struct char_data *ch);

/* Perk command functions (step 7) */
ACMD_DECL(do_perk);
ACMD_DECL(do_myperks);

/* Perk refund/reset functions (step 10) */
bool remove_char_perk(struct char_data *ch, int perk_id, int class_id);  /* Returns TRUE if removed */
void remove_class_perks(struct char_data *ch, int class_id);              /* Remove all perks for a class */
void remove_all_perks(struct char_data *ch);                               /* Remove all perks (for respec) */
void reset_all_perk_points(struct char_data *ch);                          /* Reset all perk points to 0 */

#endif /* _PERKS_H_ */
