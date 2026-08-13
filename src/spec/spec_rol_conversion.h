/**
 * @file spec/spec_rol_conversion.h
 * Shared Realms of Luminari conversion special-procedure adapters.
 */

#ifndef LUMINARI_SPEC_ROL_CONVERSION_H
#define LUMINARI_SPEC_ROL_CONVERSION_H

#include <stdbool.h>
#include <stddef.h>

struct char_data;
struct obj_data;
struct spec_event_context;

struct rol_monster_hit_profile_view
{
  int proc_denominator;
  int base_damage;
  int damage_variance;
  int damage_dice_count;
  int damage_dice_size;
  int damage_type;
  bool fatal;
};

enum rol_bandit_demand
{
  ROL_BANDIT_DEMAND_PASS = -1,
  ROL_BANDIT_DEMAND_ATTACK = -2,
  ROL_BANDIT_DEMAND_TAKE_WAGON = -3
};

enum rol_guild_family
{
  ROL_GUILD_FAMILY_MAGE = 0,
  ROL_GUILD_FAMILY_THIEF,
  ROL_GUILD_FAMILY_WARRIOR,
  ROL_GUILD_FAMILY_CLERIC,
  ROL_GUILD_FAMILY_BARD
};

enum rol_banana_peel_outcome
{
  ROL_BANANA_PEEL_AVOID = 0,
  ROL_BANANA_PEEL_KNOCKOUT,
  ROL_BANANA_PEEL_FALL,
  ROL_BANANA_PEEL_STUMBLE,
  ROL_BANANA_PEEL_DANCE
};

enum rol_scheduled_gate_state
{
  ROL_SCHEDULED_GATE_NONE = 0,
  ROL_SCHEDULED_GATE_OPEN,
  ROL_SCHEDULED_GATE_CLOSE
};

enum rol_scheduled_naval_branch
{
  ROL_SCHEDULED_NAVAL_NONE = 0,
  ROL_SCHEDULED_NAVAL_IDLE,
  ROL_SCHEDULED_NAVAL_FIGHTING
};

enum rol_scheduled_crier_notice
{
  ROL_SCHEDULED_CRIER_NONE = 0,
  ROL_SCHEDULED_CRIER_MOONSHAE_SHIP,
  ROL_SCHEDULED_CRIER_CALIMPORT_SHIP,
  ROL_SCHEDULED_CRIER_SHOPS_OPENING,
  ROL_SCHEDULED_CRIER_SHOPS_CLOSING
};

enum rol_lich_rite_status
{
  ROL_LICH_RITE_INVALID = 0,
  ROL_LICH_RITE_WRONG_CLASS,
  ROL_LICH_RITE_INELIGIBLE_LEVEL,
  ROL_LICH_RITE_UNSAFE_FOLLOWERS,
  ROL_LICH_RITE_MISSING_OFFERINGS,
  ROL_LICH_RITE_READY
};

int rol_corpse_devourer(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_poison_bite(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_thief(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_breath_weapon_fire(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_breath_weapon_cold(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_breath_weapon_acid(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_breath_weapon_gas(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_breath_weapon_lightning(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_breath_attack_acid(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_breath_attack_lightning(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_magic_pool(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_auto_distributor(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_shadow_giant(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_mage_guild_room(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_thief_guild_room(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_warrior_guild_room(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_cleric_guild_room(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_bard_guild_room(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_waterdeep_guild_room(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_guild_guard(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_guild_guard_typed(struct spec_event_context *context);
int rol_major_beholder(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_lich_energy_drain(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_lich_rite(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_bandit(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_sister_knight(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_alert_caller(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_yggdrasil_branch(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_waterdeep_ambient(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_bloodstone_critter(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_source_periodic(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_state_periodic(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_scheduled_mobile(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_waterdeep_peacekeeper(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_weapon_proc(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_weapon_proc_typed(struct spec_event_context *context);
int rol_bloodstone_portal(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_portal_door(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_travel_portal(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_designated_follower(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_fixed_bodyguard(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_floating_pool(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_item_blocker(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_command_sentinel(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_command_sentinel_typed(struct spec_event_context *context);
int rol_toll_keeper(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_toll_keeper_typed(struct spec_event_context *context);
int rol_banana(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_banana_typed(struct spec_event_context *context);
int rol_undead_drain(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_monster_combat(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_monster_combat_typed(struct spec_event_context *context);
int rol_residual_mobile_typed(struct spec_event_context *context);
int rol_utility_room(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_utility_room_typed(struct spec_event_context *context);

enum rol_scheduled_gate_state rol_scheduled_gate_state_for_hour(int hour);
enum rol_scheduled_naval_branch rol_scheduled_naval_branch_for(bool standing, bool fighting);
enum rol_scheduled_crier_notice
rol_scheduled_crier_notice_for_hour(int hour, bool *shop_notice_sent, bool *ship_notice_sent);
int rol_scheduled_lighthouse_step(int hour, bool standing, bool *active, int *counter);

bool rol_corpse_devourer_can_consume(const struct obj_data *obj);
int rol_poison_bite_roll_ceiling(int level);
int rol_umberhulk_proc_chance(int level);
int rol_shadow_giant_spook_damage(bool save_succeeded);
bool rol_shadow_giant_spook_immune(struct char_data *target);
bool rol_shadow_giant_stun_succeeds(int level, int chance_roll, int penalty_roll);
bool rol_floating_pool_should_move(int roll);
bool rol_class_guild_allows(const struct char_data *ch, enum rol_guild_family family);
bool rol_waterdeep_guild_allows(int room_vnum, const struct char_data *ch);
bool rol_guild_guard_allows(int room_vnum, int direction, const struct char_data *ch);
bool rol_guild_guard_protects(int room_vnum);
int rol_guild_guard_passage_destination(int room_vnum, int direction);
bool rol_guild_guard_trips_rejected(int room_vnum, int direction);
int rol_major_beholder_eye_spell(int eye);
int rol_major_beholder_eye_cooldown(int state, int eye);
int rol_major_beholder_advance_cooldowns(int state, unsigned int fired_eye_mask);
bool rol_lich_energy_drain_together(const struct char_data *candidate,
                                    const struct char_data *primary);
int rol_lich_energy_drain_victim_hit(int current_hit, bool death_warded);
int rol_lich_energy_drain_healer_hit(int current_hit, int drained_hit, bool blackmantled);
long rol_lich_energy_drain_stun_duration(long remaining);
enum rol_lich_rite_status rol_lich_rite_requirements(const struct char_data *ch,
                                                     struct char_data *keeper,
                                                     struct obj_data **first_offering,
                                                     struct obj_data **second_offering);
int rol_bandit_cargo_value(struct char_data *ch);
int rol_bandit_fee_gold(int target_vnum, int cargo_value, int alignment, int carried_gold);
bool rol_sister_knight_vnum(int vnum);
const char *rol_alert_message(int caller_vnum);
bool rol_alert_helper_matches(int caller_vnum, int helper_vnum);
int rol_alert_max_distance(int caller_vnum);
bool rol_yggdrasil_vnum(int vnum);
int rol_yggdrasil_release_move(int current_move);
int rol_waterdeep_ambient_roll_sides(int mobile_vnum);
bool rol_waterdeep_ambient_room_allows(int mobile_vnum, int room_vnum);
bool rol_waterdeep_ambient_fighting_allows(int mobile_vnum, bool fighting);
const char *rol_waterdeep_ambient_message(int mobile_vnum, int roll, int message_index,
                                          bool *speech);
const char *rol_conversion_death_message(int vnum);
bool rol_conversion_death_suppresses_corpse(int vnum);
int rol_conversion_death_replacement_vnum(int vnum);
int rol_conversion_death_object_vnum(int vnum);
int rol_weevil_death_adjust_damage(int damage_amount, bool fire_protected);
bool rol_conversion_death_retargets_clerics(int vnum);
long event_rol_yggdrasil_release(void *event_obj);
long event_rol_barbazu_bloodloss(void *event_obj);
const char *rol_bloodstone_critter_social(int roll);
size_t rol_source_periodic_profile_count(void);
bool rol_source_periodic_profile_bounds(int mobile_vnum, int *roll_min, int *roll_max,
                                        bool *requires_awake, bool *suppresses_fighting);
bool rol_source_periodic_dice_shape(int mobile_vnum, int *dice_count, int *dice_sides);
bool rol_source_periodic_requires_sleeping(int mobile_vnum);
int rol_source_periodic_devour_order(int mobile_vnum);
size_t rol_source_periodic_outcome_action_count(int mobile_vnum, int roll);
const char *rol_source_periodic_outcome_action(int mobile_vnum, int roll, size_t action_index,
                                               bool *speech, bool *hide);
size_t rol_state_periodic_profile_count(void);
bool rol_state_periodic_dice(int mobile_vnum, bool fighting, int *dice_count, int *dice_sides);
bool rol_state_periodic_runs_idle_while_fighting(int mobile_vnum);
size_t rol_state_periodic_outcome_action_count(int mobile_vnum, bool fighting, int roll);
const char *rol_state_periodic_outcome_action(int mobile_vnum, bool fighting, int roll,
                                              size_t action_index, bool *speech, bool *hide);
int rol_waterdeep_bouncer_home_vnum(int mobile_vnum);
size_t rol_waterdeep_bouncer_route_length(int mobile_vnum);
size_t rol_weapon_profile_count(void);
bool rol_weapon_profile(int object_vnum, int *proc_denominator, bool *critical_only,
                        const char **description);
bool rol_scornubel_fiery_mace_roll_fires(int roll);
int rol_scornubel_fiery_mace_damage(void);
bool rol_balor_weapon_profile(int object_vnum, int *dice_count, int *dice_size, int *damage_type);
bool rol_balor_weapon_owner_allowed(const struct char_data *ch, bool allow_pet);
bool rol_avernus_weapon_profile(int object_vnum, bool *barbazu_glaive, bool *gelugon_freeze_spear);
bool rol_gelugon_freeze_spear_roll_fires(int roll);
int rol_barbazu_bloodloss_next_hit(int current_hit);
bool rol_bloodstone_portal_survives(int current_hit, int hit_loss);
bool rol_portal_door_race_allows(bool rejects_good, int race);
int rol_travel_portal_destination_slot(int object_vnum, int roll);
int rol_travel_portal_fixed_destination(int object_vnum);
int rol_travel_portal_reward_vnum(int object_vnum);
bool rol_travel_portal_actor_allowed(int object_vnum, const struct char_data *ch);
bool rol_fixed_bodyguard_protects(int bodyguard_vnum, int protected_vnum);
bool rol_command_sentinel_blocks_passage(int mobile_vnum, int room_vnum, int direction,
                                         const struct char_data *ch, int chance_roll);
bool rol_command_sentinel_is_necromancer(const struct char_data *ch);
int rol_command_sentinel_glyph_damage(const struct char_data *ch);
int rol_toll_keeper_fee_gold(int mobile_vnum);
int rol_toll_keeper_destination(int mobile_vnum, bool first_side);
bool rol_toll_keeper_ticket_matches(int mobile_vnum, int room_vnum, int entered_object_vnum,
                                    int ticket_vnum);
bool rol_toll_keeper_payment_syntax_valid(int mobile_vnum, const char *argument);
enum rol_banana_peel_outcome rol_banana_peel_classify(int intelligence_roll, int dexterity_roll);
bool rol_undead_drain_profile(int mobile_vnum, int *chance_sides, int *marker_affect,
                              int *armor_penalty, int *dexterity_penalty, int *strength_penalty,
                              int *will_penalty, int *fortitude_penalty, int *slow_duration);
size_t rol_monster_combat_profile_count(void);
bool rol_monster_combat_profile(int mobile_vnum, int *proc_denominator, const char **description);
bool rol_griffon_guard_target_allowed(const struct char_data *target);
bool rol_planar_death_profile(int mobile_vnum, bool *suppresses_corpse);
bool rol_planar_burst_profile(int mobile_vnum, bool *screech, bool *spores, bool *flame_spikes);
bool rol_avernus_barbazu_profile(int mobile_vnum);
bool rol_avernus_gelugon_profile(int mobile_vnum, bool *freezing_tail, bool *silencing_bolt,
                                 bool *blocks_disarm);
bool rol_avernus_barbazu_berserk_roll_fires(int roll);
bool rol_avernus_gelugon_tail_roll_fires(int roll);
bool rol_avernus_meritos_silence_roll_fires(int roll);
enum rol_planar_control_kind
{
  ROL_PLANAR_CONTROL_NONE = 0,
  ROL_PLANAR_CONTROL_GLABREZU,
  ROL_PLANAR_CONTROL_MARILITH,
  ROL_PLANAR_CONTROL_SUCCUBUS
};
bool rol_planar_control_profile(int mobile_vnum, enum rol_planar_control_kind *kind,
                                int *proc_denominator);
bool rol_planar_captive_command_allowed(const char *command);
bool rol_planar_restrain_agility_evades(int agility, int roll);
bool rol_planar_restrain_constitution_survives(int constitution, int roll);
bool rol_planar_succubus_charm_roll_fires(int roll);
int rol_planar_succubus_kiss_delay_seconds(int hours);
bool rol_planar_vrock_dance_profile(int mobile_vnum);
int rol_planar_vrock_dance_required_count(void);
int rol_planar_vrock_dance_step_seconds(void);
int rol_planar_vrock_dance_cooldown_seconds(void);
bool rol_planar_five_in_six_roll_fires(int roll);
bool rol_planar_screech_health_allows(int hit, int max_hit);
int rol_planar_screech_cooldown_seconds(int mobile_vnum);
int rol_planar_hit_burst_cooldown_seconds(int mobile_vnum);
bool rol_monster_successful_hit_profile(int mobile_vnum, struct rol_monster_hit_profile_view *view);
bool rol_monster_successful_hit_roll_fires(int mobile_vnum, int roll);
bool rol_skriaxit_sandstorm_profile(int mobile_vnum, int *round_interval,
                                    bool *reaches_open_adjacent_rooms);
int rol_skriaxit_sandstorm_source_damage(int skriaxit_count);
int rol_skriaxit_sandstorm_advance_round(int current_round, bool *fires);
bool rol_seelie_faerie_profile(int mobile_vnum, bool *faerie_fire, bool *prismatic, bool *search);
bool rol_seelie_faerie_runs_while_disabled(int mobile_vnum);
int rol_seelie_prismatic_beam_count(int roll);
int rol_seelie_prismatic_damage(int color);
int rol_seelie_search_stun_rounds(int mobile_vnum);
bool rol_manscorpion_venom_profile(int mobile_vnum, int *proc_denominator, int *duration,
                                   bool *fatal_without_slow_poison);
bool rol_manscorpion_venom_roll_fires(int mobile_vnum, int roll);
bool rol_manscorpion_apply_venom(struct char_data *victim, int duration);
size_t rol_residual_mobile_profile_count(void);
bool rol_residual_mobile_profile(int mobile_vnum, const char **description);
int rol_planar_gate_cooldown_seconds(const struct char_data *ch);
bool rol_automatic_race_activity(struct char_data *ch);
void rol_automatic_race_combat_turn(struct char_data *ch);
int rol_dissolve_abyss_forged_weapons(struct char_data *ch);
bool rol_handle_conjured_death(struct char_data *ch);
bool rol_update_mobile_home_after_move(struct char_data *ch, int source_room, int destination_room);
int rol_utility_newbie_east_destination_vnum(int race);
int rol_utility_weight_transition(bool triggered, unsigned long long weight);

#endif /* LUMINARI_SPEC_ROL_CONVERSION_H */
