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
void define_alchemist_perks(void);
void define_psionicist_perks(void);
void define_blackguard_perks(void);
void define_inquisitor_perks(void);

/* Inquisitor helper functions - Judgment & Spellcasting Tree Tier 1 */
int get_inquisitor_empowered_judgment_bonus(struct char_data *ch);
bool can_inquisitor_dual_judgment(struct char_data *ch);
bool has_inquisitor_swift_spellcaster(struct char_data *ch);
int get_inquisitor_divination_dc_bonus(struct char_data *ch);
bool has_inquisitor_divination_bonus_slot(struct char_data *ch);
bool has_inquisitor_judgment_recovery(struct char_data *ch);

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

/* Alchemist Mutagenist helper functions */
int get_alchemist_mutagen_i_rank(struct char_data *ch);
int get_alchemist_hardy_constitution_hp_bonus(struct char_data *ch);
bool has_alchemist_alchemical_reflexes(struct char_data *ch);
bool has_alchemist_natural_armor(struct char_data *ch);
int get_alchemist_mutagen_ii_rank(struct char_data *ch);
bool has_alchemist_persistence_mutagen(struct char_data *ch);
bool has_alchemist_infused_with_vigor(struct char_data *ch);
bool has_alchemist_cellular_adaptation(struct char_data *ch);
/* Tier III */
bool has_alchemist_improved_mutagen(struct char_data *ch);
bool is_alchemist_unstable_mutagen_on(struct char_data *ch);
bool is_alchemist_universal_mutagen_ready(struct char_data *ch);
int get_alchemist_mutagenic_mastery_bonus(struct char_data *ch);

/* Tier III */
bool has_alchemist_improved_mutagen(struct char_data *ch);
bool is_alchemist_unstable_mutagen_on(struct char_data *ch);
bool is_alchemist_universal_mutagen_ready(struct char_data *ch);
int get_alchemist_mutagenic_mastery_bonus(struct char_data *ch);

/* Mutagenist Tier IV helpers */
bool has_alchemist_perfect_mutagen(struct char_data *ch);
bool can_use_chimeric_transmutation(struct char_data *ch);
void use_chimeric_transmutation(struct char_data *ch);
void update_chimeric_transmutation_combat_end(struct char_data *ch);

/* Bomb Craftsman Tier I helpers */
int get_alchemist_bomb_damage_bonus(struct char_data *ch);
int get_alchemist_bomb_precision_bonus(struct char_data *ch);
int get_alchemist_bomb_splash_damage_bonus(struct char_data *ch);
int get_alchemist_bomb_dc_bonus(struct char_data *ch);
int get_alchemist_quick_bomb_chance(struct char_data *ch);

/* Bomb Craftsman Tier II helpers */
int get_alchemist_bomb_damage_bonus_tier2(struct char_data *ch);
bool has_alchemist_elemental_bomb(struct char_data *ch);
int get_alchemist_elemental_bomb_bypass(struct char_data *ch, int dam_type);
int get_alchemist_elemental_bomb_extra_damage(struct char_data *ch, int dam_type);
bool has_alchemist_concussive_bomb(struct char_data *ch);
bool has_alchemist_poison_bomb(struct char_data *ch);

/* Bomb Craftsman Tier III helpers */
bool has_alchemist_inferno_bomb(struct char_data *ch);
bool has_alchemist_cluster_bomb(struct char_data *ch);
int get_alchemist_calculated_throw_dc_bonus(struct char_data *ch);
bool has_alchemist_bomb_mastery(struct char_data *ch);
int get_alchemist_bomb_mastery_damage_bonus(struct char_data *ch);

/* Bomb Craftsman Tier IV helpers */
bool has_alchemist_bombardier_savant(struct char_data *ch);
int get_bombardier_savant_attack_bonus(struct char_data *ch);
int get_bombardier_savant_damage_bonus(struct char_data *ch);
bool has_alchemist_volatile_catalyst(struct char_data *ch);
bool is_volatile_catalyst_on(struct char_data *ch);

/* Extract Master Tier I helpers */
int get_alchemist_extract_i_rank(struct char_data *ch);
int get_alchemist_extract_not_consumed_chance(struct char_data *ch);
int get_alchemist_infusion_i_rank(struct char_data *ch);
int get_alchemist_infusion_dc_bonus(struct char_data *ch);
bool has_alchemist_swift_extraction(struct char_data *ch);
bool has_alchemist_resonant_extract(struct char_data *ch);

/* Extract Master Tier II helpers */
int get_alchemist_extract_ii_rank(struct char_data *ch);
int get_alchemist_infusion_ii_rank(struct char_data *ch);
bool has_alchemist_concentrated_essence(struct char_data *ch);
bool has_alchemist_persistent_extraction(struct char_data *ch);

/* Extract Master Tier III helpers */
bool has_alchemist_healing_extraction(struct char_data *ch);
int get_alchemist_healing_extraction_amount(struct char_data *ch);
bool has_alchemist_alchemical_compatibility(struct char_data *ch);
bool has_alchemist_discovery_extraction(struct char_data *ch);
bool has_alchemist_master_alchemist(struct char_data *ch);

/* Extract Master Tier IV helpers */
bool has_alchemist_eternal_extract(struct char_data *ch);
bool has_alchemist_quintessential_extraction(struct char_data *ch);

/* Psionicist Telepathic Control Tier I helpers */
int get_psionic_telepathy_dc_bonus(struct char_data *ch);
int get_psionic_telepathy_penetration_bonus(struct char_data *ch);
bool has_psionic_suggestion_primer(struct char_data *ch);
bool has_psionic_focus_channeling(struct char_data *ch);

/* Psionicist Telepathic Control Tier I mechanics */
void apply_psionic_suggestion_primer(struct char_data *ch, struct char_data *vict, int spellnum, int routines_flags);
void apply_psionic_focus_channeling(struct char_data *ch);

/* Psionicist Telepathic Control Tier II helpers */
bool has_mind_spike_ii_bonus(struct char_data *ch, int augment_spent);
bool has_overwhelm(struct char_data *ch);
bool overwhelm_used_this_combat(struct char_data *ch);
void set_overwhelm_cooldown(struct char_data *ch);
bool has_linked_menace(struct char_data *ch);

/* Psionicist Telepathic Control Tier II mechanics */
void apply_linked_menace_ac_penalty(struct char_data *vict);

/* Psionicist Telepathic Control Tier III helpers */
bool has_psionic_dominion(struct char_data *ch);
bool has_psychic_sundering(struct char_data *ch);
bool has_psionic_mental_backlash(struct char_data *ch);
bool has_psionic_piercing_will(struct char_data *ch);
void apply_psionic_dominion_extension(struct char_data *ch, struct char_data *vict, int spellnum, struct affected_type *af_array, int count);
void apply_psychic_sundering_debuff(struct char_data *ch, struct char_data *vict);
int get_psionic_piercing_will_bonus(struct char_data *ch);
int get_psionic_mental_backlash_damage(struct char_data *ch, int level);

/* Psionicist Telepathic Control Tier IV helpers */
bool has_psionic_absolute_geas(struct char_data *ch);
bool has_psionic_hive_commander(struct char_data *ch);
void apply_absolute_geas_debuffs(struct char_data *ch, struct char_data *vict, int level);
void apply_hive_commander_mark(struct char_data *ch, struct char_data *vict);

/* Psionicist Psychokinetic Arsenal Tier I helpers */
bool has_kinetic_edge_i(struct char_data *ch);
bool has_force_screen_adept(struct char_data *ch);
bool has_vector_shove(struct char_data *ch);
bool has_energy_specialization(struct char_data *ch);
int get_kinetic_edge_bonus(struct char_data *ch);
int get_force_screen_ac_bonus(struct char_data *ch);
int get_force_screen_duration_bonus(struct char_data *ch);
int get_vector_shove_movement_bonus(struct char_data *ch);
int get_vector_shove_damage_bonus(struct char_data *ch);
int get_energy_specialization_dc_bonus(struct char_data *ch, int element);

/* Tier 2 Psychokinetic Arsenal helpers */
bool has_kinetic_edge_ii(struct char_data *ch);
int get_kinetic_edge_ii_bonus(struct char_data *ch);

bool has_deflective_screen(struct char_data *ch);
int get_deflective_screen_ranged_ac_bonus(struct char_data *ch);
int get_deflective_screen_reflex_bonus(struct char_data *ch);
int get_deflective_screen_first_hit_dr(struct char_data *ch);

bool has_accelerated_manifestation(struct char_data *ch);

bool has_energy_retort_perk(struct char_data *ch);
int get_energy_retort_bonus_damage(struct char_data *victim);

/* Tier 3 Psychokinetic Arsenal helpers */
bool has_kinetic_edge_iii(struct char_data *ch);
int get_kinetic_edge_iii_bonus(struct char_data *ch);
int get_kinetic_edge_iii_dc_bonus(struct char_data *ch, int spellnum);

bool has_gravity_well(struct char_data *ch);
bool can_use_gravity_well(struct char_data *ch);
void use_gravity_well(struct char_data *ch);

bool has_force_aegis(struct char_data *ch);
int get_force_aegis_ranged_ac_bonus(struct char_data *ch);
int get_force_aegis_temp_hp_bonus(struct char_data *ch);

bool has_kinetic_crush(struct char_data *ch);
bool should_apply_kinetic_crush_prone(struct char_data *ch);
int get_kinetic_crush_collision_damage(struct char_data *ch);

/* Tier 4 Psychokinetic Arsenal helpers */
bool has_singular_impact(struct char_data *ch);
bool can_use_singular_impact(struct char_data *ch);
void use_singular_impact(struct char_data *ch, struct char_data *victim);

bool has_perfect_deflection(struct char_data *ch);
bool can_use_perfect_deflection(struct char_data *ch);
void use_perfect_deflection(struct char_data *ch);
/* Metacreative Genius Tier I helpers */
bool has_ectoplasmic_artisan_i(struct char_data *ch);
bool can_use_ectoplasmic_artisan_psp_reduction(struct char_data *ch);
void use_ectoplasmic_artisan_psp_reduction(struct char_data *ch);
int get_ectoplasmic_artisan_psp_reduction(struct char_data *ch);
int get_ectoplasmic_artisan_duration_bonus(struct char_data *ch);

/* Metacreative Genius Tier II helpers */
bool has_ectoplasmic_artisan_ii(struct char_data *ch);
bool has_shardstorm(struct char_data *ch);
bool has_hardened_constructs_ii(struct char_data *ch);
int get_hardened_constructs_ii_ac_bonus(struct char_data *ch);
int get_hardened_constructs_dr_amount(struct char_data *ch);
bool has_rapid_manifester(struct char_data *ch);
bool can_use_rapid_manifester(struct char_data *ch);
void use_rapid_manifester(struct char_data *ch);

/* Metacreative Genius - Tier III */
bool has_ectoplasmic_artisan_iii(struct char_data *ch);
int get_ectoplasmic_artisan_iii_psp_reduction(struct char_data *ch);
int get_ectoplasmic_artisan_iii_duration_bonus(struct char_data *ch);
bool can_use_ectoplasmic_artisan_iii_psp_reduction(struct char_data *ch);
void use_ectoplasmic_artisan_iii_psp_reduction(struct char_data *ch);
bool has_empowered_creation(struct char_data *ch);
bool has_construct_commander(struct char_data *ch);
int get_construct_commander_summon_bonus(struct char_data *ch);
bool has_self_forged(struct char_data *ch);
int get_self_forged_temp_hp(struct char_data *ch);

/* Metacreative Genius - Tier IV (Capstones) */
bool has_astral_juggernaut(struct char_data *ch);
bool can_use_astral_juggernaut(struct char_data *ch);
void use_astral_juggernaut(struct char_data *ch);
bool has_perfect_fabricator(struct char_data *ch);
bool can_use_perfect_fabricator(struct char_data *ch);
void use_perfect_fabricator(struct char_data *ch);

bool has_shard_volley(struct char_data *ch);
bool should_add_extra_shard_projectile(struct char_data *ch, int augment_psp);

bool has_hardened_constructs_i(struct char_data *ch);
int get_hardened_constructs_temp_hp(struct char_data *ch);
int get_hardened_constructs_ac_bonus(struct char_data *ch);

bool has_fabricate_focus(struct char_data *ch);
int get_fabricate_focus_casting_time_reduction(struct char_data *ch);

/* Blackguard Tyranny & Fear Tier I–II helpers */
int get_blackguard_dread_presence_intimidate_bonus(struct char_data *ch);
int get_blackguard_extra_fear_aura_penalty(struct char_data *vict);
bool has_blackguard_intimidating_smite(struct char_data *ch);
int get_blackguard_cruel_edge_damage_bonus(struct char_data *ch, struct char_data *vict);
bool has_blackguard_command_the_weak(struct char_data *ch);
bool can_use_command_the_weak_swift(struct char_data *ch);
void use_command_the_weak_swift(struct char_data *ch);
bool has_blackguard_terror_tactics(struct char_data *ch);
bool has_blackguard_nightmarish_visage(struct char_data *ch);

/* Blackguard Tyranny & Fear Tier III–IV helpers */
bool has_blackguard_paralyzing_dread(struct char_data *ch);
bool try_paralyzing_dread(struct char_data *vict);
bool has_blackguard_despair_harvest(struct char_data *ch);
void apply_despair_harvest(struct char_data *ch, struct char_data *vict);
bool has_blackguard_shackles_of_awe(struct char_data *ch);
int get_shackles_of_awe_attack_penalty(struct char_data *ch, struct char_data *vict);
int get_shackles_of_awe_speed_penalty(struct char_data *ch, struct char_data *vict);
bool has_blackguard_profane_dominion(struct char_data *ch);
int get_profane_dominion_damage(struct char_data *ch, struct char_data *vict);
bool has_blackguard_sovereign_of_terror(struct char_data *ch);
void apply_sovereign_fear_escalation(struct char_data *ch, struct char_data *vict);
bool has_blackguard_midnight_edict(struct char_data *ch);
bool can_use_midnight_edict(struct char_data *ch);
bool perform_midnight_edict(struct char_data *ch);
bool is_cowering(struct char_data *ch);

/* Blackguard Profane Might Tier I–II helpers */
int get_blackguard_vile_strike_damage(struct char_data *ch, struct char_data *vict);
bool has_blackguard_cruel_momentum(struct char_data *ch);
int get_blackguard_cruel_momentum_damage(struct char_data *ch);
void apply_blackguard_cruel_momentum_stack(struct char_data *ch);
bool has_blackguard_dark_channel(struct char_data *ch);
int get_blackguard_dark_channel_damage(struct char_data *ch);
bool has_blackguard_brutal_oath(struct char_data *ch);
int get_blackguard_brutal_oath_bonus(struct char_data *ch, struct char_data *vict);
bool has_blackguard_ravaging_smite(struct char_data *ch);
void apply_blackguard_ravaging_smite(struct char_data *ch, struct char_data *vict);
bool has_blackguard_profane_weapon_bond(struct char_data *ch);
bool can_use_profane_weapon_bond(struct char_data *ch);
bool activate_profane_weapon_bond(struct char_data *ch);
bool has_blackguard_relentless_assault(struct char_data *ch);
bool can_trigger_relentless_assault(struct char_data *ch);
void trigger_relentless_assault(struct char_data *ch);
bool has_blackguard_sanguine_barrier(struct char_data *ch);
void apply_blackguard_sanguine_barrier(struct char_data *ch, int damage);

/* Blackguard Profane Might Tier III–IV helpers */
bool has_blackguard_doom_cleave(struct char_data *ch);
bool has_blackguard_soul_rend(struct char_data *ch);
int get_blackguard_soul_rend_bonus(struct char_data *ch, struct char_data *vict);
bool has_blackguard_blackened_precision(struct char_data *ch);
bool has_blackguard_unholy_blitz(struct char_data *ch);
bool can_use_unholy_blitz(struct char_data *ch);
bool has_blackguard_avatar_of_profanity(struct char_data *ch);
bool has_blackguard_cataclysmic_smite(struct char_data *ch);

/* Blackguard Unholy Resilience Tier 1-2 helpers */
bool has_blackguard_profane_fortitude(struct char_data *ch);
int get_blackguard_profane_fortitude_bonus(struct char_data *vict, struct char_data *caster);
bool has_blackguard_dark_aegis(struct char_data *ch);
int get_blackguard_dark_aegis_dr(struct char_data *ch);
bool has_blackguard_graveborn_vigor(struct char_data *ch);
void trigger_blackguard_graveborn_vigor(struct char_data *ch);
bool has_blackguard_sinister_recovery(struct char_data *ch);
bool has_blackguard_aura_of_desecration(struct char_data *ch);
bool has_blackguard_fell_ward(struct char_data *ch);
bool has_blackguard_defiant_hide(struct char_data *ch);
bool has_blackguard_shade_step(struct char_data *ch);

/* Blackguard Unholy Resilience Tier 3-4 helpers */
bool has_blackguard_necrotic_regeneration(struct char_data *ch);
int get_blackguard_necrotic_regeneration(struct char_data *ch);
bool has_blackguard_unholy_fortification(struct char_data *ch);
bool has_blackguard_blasphemous_warding(struct char_data *ch);
int get_blackguard_blasphemous_warding_sr(struct char_data *ch, struct char_data *caster, int spellnum);
bool has_blackguard_resilient_corruption(struct char_data *ch);
int get_blackguard_resilient_corruption_dr(struct char_data *ch);
void increment_blackguard_resilient_corruption(struct char_data *ch);
void reset_blackguard_resilient_corruption(struct char_data *ch);
bool has_blackguard_undying_vigor(struct char_data *ch);
bool trigger_blackguard_undying_vigor(struct char_data *ch);

/* New Blackguard Unholy Resilience Tier 3 helpers */
bool has_blackguard_soul_carapace(struct char_data *ch);
void apply_blackguard_soul_carapace(struct char_data *ch, int damage);
bool has_blackguard_warding_malice(struct char_data *ch);
int get_blackguard_warding_malice_penalty(struct char_data *vict, struct char_data *caster);
bool has_blackguard_blackguards_reprisal(struct char_data *ch);
void trigger_blackguard_reprisal_on_save(struct char_data *ch, int casttype);
int get_blackguard_reprisal_damage_bonus(struct char_data *ch, struct char_data *vict);

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
bool has_shared_spells(struct char_data *ch);
int get_ranger_conjuration_dc_bonus(struct char_data *ch);
int get_greater_summons_hp_bonus(struct char_data *ch);
int get_greater_summons_damage(struct char_data *ch);
int get_greater_summons_attack_bonus(struct char_data *ch);

/* Ranger Wilderness Warrior perk functions */
int get_ranger_two_weapon_focus_tohit(struct char_data *ch);
int get_ranger_two_weapon_focus_damage(struct char_data *ch);
int get_ranger_dual_strike_offhand(struct char_data *ch);
int get_ranger_favored_enemy_mastery_damage(struct char_data *ch);
int get_ranger_toughness_hp(struct char_data *ch);
int get_ranger_tempest_ac(struct char_data *ch);
int get_ranger_favored_enemy_slayer_tohit(struct char_data *ch);
int get_ranger_favored_enemy_slayer_crit(struct char_data *ch);

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

/* Bard Spellsinger Tree Perk Functions */
int get_bard_enchanters_guile_dc_bonus(struct char_data *ch);
int get_bard_songweaver_level_bonus(struct char_data *ch);
int get_bard_resonant_voice_save_bonus(struct char_data *ch);
bool has_bard_harmonic_casting(struct char_data *ch);
int get_bard_songweaver_ii_level_bonus(struct char_data *ch);
int get_bard_enchanters_guile_ii_dc_bonus(struct char_data *ch);
bool has_bard_crescendo(struct char_data *ch);
int get_bard_crescendo_sonic_damage(struct char_data *ch);
int get_bard_crescendo_dc_bonus(struct char_data *ch);
bool has_bard_sustaining_melody(struct char_data *ch);

/* Bard Spellsinger Tree Tier 3 Functions */
bool has_bard_master_of_motifs(struct char_data *ch);
bool has_bard_dirge_of_dissonance(struct char_data *ch);
int get_bard_dirge_sonic_damage(struct char_data *ch);
int get_bard_dirge_concentration_penalty(struct char_data *ch);
bool has_bard_heightened_harmony(struct char_data *ch);
int get_bard_heightened_harmony_perform_bonus(struct char_data *ch);
bool has_bard_protective_chorus(struct char_data *ch);
int get_bard_protective_chorus_save_bonus(struct char_data *ch);
int get_bard_protective_chorus_ac_bonus(struct char_data *ch);

/* Bard Spellsinger Tree Tier 4 Functions - Capstones */
bool has_bard_spellsong_maestra(struct char_data *ch);
int get_bard_spellsong_maestra_caster_bonus(struct char_data *ch);
int get_bard_spellsong_maestra_dc_bonus(struct char_data *ch);
bool has_bard_spellsong_maestra_metamagic_free(struct char_data *ch);

bool has_bard_aria_of_stasis(struct char_data *ch);
int get_bard_aria_stasis_ally_saves_bonus(struct char_data *ch);
int get_bard_aria_stasis_enemy_tohit_penalty(struct char_data *ch);
int get_bard_aria_stasis_movement_penalty(struct char_data *ch);

bool has_bard_symphonic_resonance(struct char_data *ch);
int get_bard_symphonic_resonance_temp_hp(struct char_data *ch);
int get_bard_symphonic_resonance_daze_duration(struct char_data *ch);
int get_bard_symphonic_resonance_daze_range(struct char_data *ch);

bool has_bard_endless_refrain(struct char_data *ch);
int get_bard_endless_refrain_slot_regen(struct char_data *ch);
bool should_endless_refrain_consume_performance(struct char_data *ch);

/* Bard Warchanter Tree Tier 1 Functions */
int get_bard_battle_hymn_damage_bonus(struct char_data *ch);
int get_bard_drummers_rhythm_tohit_bonus(struct char_data *ch);
bool has_bard_rallying_cry_perk(struct char_data *ch);
int get_bard_rallying_cry_fear_save_bonus(struct char_data *ch);
bool has_bard_frostbite_refrain(struct char_data *ch);
int get_bard_frostbite_cold_damage(struct char_data *ch);
int get_bard_frostbite_natural_20_debuff(struct char_data *ch);

/* Bard Warchanter Tree Tier 2 Functions */
int get_bard_battle_hymn_ii_damage_bonus(struct char_data *ch);
int get_bard_drummers_rhythm_ii_tohit_bonus(struct char_data *ch);
bool has_bard_warbeat(struct char_data *ch);
int get_bard_warbeat_ally_damage_bonus(struct char_data *ch);
bool has_bard_frostbite_refrain_ii(struct char_data *ch);
int get_bard_frostbite_refrain_ii_cold_damage(struct char_data *ch);
int get_bard_frostbite_refrain_ii_natural_20_debuff_attack(struct char_data *ch);
int get_bard_frostbite_refrain_ii_natural_20_debuff_ac(struct char_data *ch);

/* Bard Warchanter Tree Tier 3 Functions */
bool has_bard_anthem_of_fortitude(struct char_data *ch);
int get_bard_anthem_fortitude_hp_bonus(struct char_data *ch);
int get_bard_anthem_fortitude_save_bonus(struct char_data *ch);
bool has_bard_commanding_cadence(struct char_data *ch);
int get_bard_commanding_cadence_daze_chance(struct char_data *ch);
bool has_bard_steel_serenade(struct char_data *ch);
int get_bard_steel_serenade_ac_bonus(struct char_data *ch);
int get_bard_steel_serenade_damage_resistance(struct char_data *ch);
bool has_bard_banner_verse(struct char_data *ch);
int get_bard_banner_verse_tohit_bonus(struct char_data *ch);
int get_bard_banner_verse_save_bonus(struct char_data *ch);

/* Bard Warchanter Tree Tier 4 Functions */
bool has_bard_warchanters_dominance(struct char_data *ch);
int get_bard_warchanters_dominance_tohit_bonus(struct char_data *ch);
int get_bard_warchanters_dominance_ac_bonus(struct char_data *ch);
int get_bard_warchanters_dominance_damage_bonus(struct char_data *ch);
bool has_bard_winters_war_march(struct char_data *ch);
int get_bard_winters_war_march_damage(struct char_data *ch);

/* Bard Swashbuckler Tree Tier 1 Functions */
bool has_bard_fencers_footwork_i(struct char_data *ch);
int get_bard_fencers_footwork_ac_bonus(struct char_data *ch);
int get_bard_fencers_footwork_reflex_bonus(struct char_data *ch);
bool has_bard_precise_strike_i(struct char_data *ch);
int get_bard_precise_strike_i_bonus(struct char_data *ch);
bool has_bard_riposte_training_i(struct char_data *ch);
int get_bard_riposte_training_i_chance(struct char_data *ch);
bool has_bard_flourish_perk(struct char_data *ch);
bool is_affected_by_flourish(struct char_data *ch);
int get_bard_flourish_tohit_bonus(struct char_data *ch);
int get_bard_flourish_ac_bonus(struct char_data *ch);

/* Bard Swashbuckler Tree Tier 2 Functions */
bool has_bard_fencers_footwork_ii(struct char_data *ch);
int get_bard_fencers_footwork_ii_ac_bonus(struct char_data *ch);
int get_bard_fencers_footwork_ii_reflex_bonus(struct char_data *ch);
bool has_bard_precise_strike_ii(struct char_data *ch);
int get_bard_precise_strike_ii_bonus(struct char_data *ch);
bool has_bard_duelists_poise(struct char_data *ch);
int get_bard_duelists_poise_crit_confirm_bonus(struct char_data *ch);
int get_bard_duelists_poise_threat_range_bonus(struct char_data *ch);
bool has_bard_agile_disengage(struct char_data *ch);
bool is_affected_by_agile_disengage(struct char_data *ch);
int get_bard_agile_disengage_ac_bonus(struct char_data *ch);

/* Bard Swashbuckler Tree Tier 3 Functions */
bool has_bard_perfect_tempo(struct char_data *ch);
bool is_affected_by_perfect_tempo(struct char_data *ch);
int get_bard_perfect_tempo_tohit_bonus(struct char_data *ch);
int get_bard_perfect_tempo_damage_bonus(struct char_data *ch);
bool has_bard_showstopper(struct char_data *ch);
bool has_bard_acrobatic_charge(struct char_data *ch);
int get_bard_acrobatic_charge_tohit_bonus(struct char_data *ch);
bool has_bard_feint_and_finish(struct char_data *ch);
bool is_affected_by_feint_and_finish(struct char_data *ch);
int get_bard_feint_and_finish_damage_bonus(struct char_data *ch);
int get_bard_feint_and_finish_crit_confirm_bonus(struct char_data *ch);

/* Tier 4 Swashbuckler Capstone Perks */
bool has_bard_supreme_style(struct char_data *ch);
bool is_affected_by_supreme_style(struct char_data *ch);
int get_bard_supreme_style_tohit_bonus(struct char_data *ch);
int get_bard_supreme_style_ac_bonus(struct char_data *ch);
int get_bard_supreme_style_crit_confirm_bonus(struct char_data *ch);
bool has_bard_curtain_call(struct char_data *ch);
bool is_affected_by_curtain_call(struct char_data *ch);
bool is_affected_by_curtain_call_disoriented(struct char_data *ch);
int get_bard_curtain_call_damage_bonus(struct char_data *ch);

#endif /* _PERKS_H_ */
