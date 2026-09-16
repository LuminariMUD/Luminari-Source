# LuminariMUD Makefile.am - for use with automake
# Based on the original Makefile.in structure

# Set up subdirectories
SUBDIRS = util

# Additional compiler flags
# Revert to project defaults: do NOT force global feature defines here.
# The production profile lives in CFLAGS/LDFLAGS (configure appends it after
# the user's flags) rather than here, where user CFLAGS would override it.
AM_CFLAGS = @CSTD_CFLAGS@ @MYFLAGS@ @LIBEVENT_CFLAGS@
# Source includes resolve from src/. The generated conf.h and build_identity.h
# sit in the build root, which Automake's DEFAULT_INCLUDES already provides.
AM_CPPFLAGS = -I$(top_srcdir)/src
# The versioned installer and the copyover identity line key on the ELF build
# ID.  Distribution compilers emit one by default; a toolchain built from
# source (the official gcc container images) does not, so ask for it.
AM_LDFLAGS = -Wl,--build-id


# Main program
bin_PROGRAMS = luminari

# Source files for the main program
luminari_SOURCES = \
	src/config/pet_vnums.h \
	src/player/account.c \
	src/act/act.comm.c \
	src/act/act.comm.do_spec_comm.c \
	src/act/act.informative.c \
	src/obj/act.item.c \
	src/movement/movement.c \
	src/movement/movement_validation.c \
	src/movement/movement_cost.c \
	src/movement/movement_position.c \
	src/movement/door_state.c \
	src/movement/movement_doors.c \
	src/movement/movement_falling.c \
	src/movement/movement_messages.c \
	src/movement/movement_tracks.c \
	src/movement/movement_events.c \
	src/combat/act.offensive.c \
	src/act/act.other.c \
	src/events/ready_action.c \
	src/combat/tactical_effects.c \
	src/combat/tactical_effects.h \
	src/act/act.social.c \
	src/act/act.wizard.c \
	src/act/act.wizard.set.c \
	src/events/actionqueues.c \
	src/events/actions.c \
	src/olc/aedit.c \
	src/ai/ai_cache.c \
	src/ai/ai_events.c \
	src/ai/ai_security.c \
	src/ai/ai_service.c \
	src/craft/alchemy.c \
	src/movement/asciimap.c \
	src/combat/assign_wpn_armor.c \
	src/combat/projectiles.c \
	src/combat/projectiles.h \
	src/character/abilities.c \
	src/character/backgrounds.c \
	src/character/guild_services.c \
	src/character/character_creation.c \
	src/character/character_creation_content.c \
	src/player/ban.c \
	src/character/bardic_performance.c \
	src/core/binary_formats.c \
	src/comms/boards.c \
	src/olc/bedit.c \
	src/comms/mysql_boards.c \
	src/core/bsd-snprintf.c \
	src/craft/brew.c \
	src/olc/cedit.c \
	src/character/char_descs.c \
	src/clan/clan.c \
	src/clan/clan_services.c \
	src/clan/clan_economy.c \
	src/olc/clan_edit.c \
	src/clan/clan_transactions.c \
	src/character/class.c \
	src/combat/combat_modes.c \
	src/core/comm.c \
	src/config/config.c \
	src/core/constants.c \
	src/core/copyover_diagnostic.c \
	src/craft/craft.c \
	src/craft/craft_training.c \
	src/craft/crafting_molds.c \
	src/craft/crafting_new.c \
	src/craft/crafting_recipes.c \
	src/craft/crafts.c \
	src/core/db.c \
	src/database/db_init.c \
	src/database/db_init_data.c \
	src/database/db_startup_init.c \
	src/database/db_admin_commands.c \
	src/character/deities.c \
	src/wilderness/desc_engine.c \
	src/dgscript/dg_comm.c \
	src/net/discord_bridge.c \
	src/dgscript/dg_db_scripts.c \
	src/dgscript/dg_event.c \
	src/dgscript/dg_handler.c \
	src/dgscript/dg_misc.c \
	src/dgscript/dg_mobcmd.c \
	src/dgscript/dg_objcmd.c \
	src/dgscript/dg_olc.c \
	src/dgscript/dg_scripts.c \
	src/dgscript/dg_triggers.c \
	src/dgscript/dg_variables.c \
	src/dgscript/dg_wldcmd.c \
	src/magic/domain_powers.c \
	src/magic/domains_schools.c \
	src/config/dotenv.c \
	src/core/elf_build_id.c \
	src/core/elf_build_id.h \
	src/combat/encounters.c \
	src/combat/combat_encounters.c \
	src/combat/combat_messages.c \
	src/combat/combat_encounters.h \
	src/combat/combat_damage.c \
	src/combat/combat_damage.h \
	src/combat/combat_death.h \
	src/combat/combat_reactions.c \
	src/combat/combat_reactions.h \
	src/combat/combat_state.c \
	src/combat/combat_state.h \
	src/character/evolutions.c \
	src/character/feats.c \
	src/character/talents.c \
	src/character/perks.c \
	src/character/perk_definitions.c \
	src/combat/fight.c \
	src/olc/genmob.c \
	src/olc/genobj.c \
	src/olc/genolc.c \
	src/olc/genqst.c \
	src/olc/genshp.c \
	src/olc/genwld.c \
	src/olc/genzon.c \
	src/events/game_scheduler.c \
	src/events/game_scheduler.h \
	src/events/event_runtime.c \
	src/events/event_runtime.h \
	src/events/event_handle.h \
	src/events/active_world.c \
	src/events/active_world.h \
	src/events/activity_manager.c \
	src/events/activity_manager.h \
	src/events/affected_owners.c \
	src/events/affected_owners.h \
	src/events/character_periodic.c \
	src/events/character_periodic.h \
	src/events/point_update_periodic.c \
	src/events/point_update_periodic.h \
	src/vessels/vessel_periodic.c \
	src/vessels/vessel_periodic.h \
	src/quest/staff_event_agenda.c \
	src/quest/staff_event_agenda.h \
	src/vessels/moving_room_events.c \
	src/vessels/moving_room_events.h \
	src/magic/buff_sequence.c \
	src/magic/buff_sequence.h \
	src/vessels/transport_jobs.c \
	src/vessels/transport_jobs.h \
	src/events/periodic_owners.c \
	src/events/periodic_owners.h \
	src/events/domain_events.c \
	src/events/domain_events.h \
	src/events/domain_event_types.c \
	src/events/domain_event_world.c \
	src/events/domain_event_world.h \
	src/events/domain_event_types.h \
	src/events/domain_event_runtime.c \
	src/events/domain_object_transfer.c \
	src/events/domain_object_transfer.h \
	src/events/domain_event_runtime.h \
	src/events/event_debug.c \
	src/events/event_debug.h \
	src/net/reactor.c \
	src/net/reactor.h \
	src/movement/graph.c \
	src/combat/grapple.c \
	src/core/handler.c \
	src/olc/hedit.c \
	src/core/help.c \
	src/core/helpers.c \
	src/olc/hlqedit.c \
	src/quest/hlquest.c \
	src/obj/house.c \
	src/olc/hsedit.c \
	src/quest/hunts.c \
	src/comms/ibt.c \
	src/olc/improved-edit.c \
	src/core/interpreter.c \
	src/character/introduce.c \
	src/wilderness/kdtree.c \
	src/core/limits.c \
	src/core/lists.c \
	src/magic/magic.c \
	src/comms/mail.c \
	src/olc/medit.c \
	src/magic/metamagic_science.c \
	src/quest/missions.c \
	src/mob/mob_act.c \
	src/mob/mob_autoroll.c \
	src/mob/mob_autoroll.h \
	src/mob/mob_autoroll_profile.c \
	src/mob/mob_memory.c \
	src/mob/mob_utils.c \
	src/mob/mob_race.c \
	src/mob/mob_psionic.c \
	src/mob/mob_class.c \
	src/mob/mob_spells.c \
	src/mob/mob_spellslots.c \
	src/mob/mob_known_spells.c \
	src/core/modify.c \
	src/magic/moon_bonus_spells.c \
	src/olc/msgedit.c \
	src/events/mud_event.c \
	src/events/mud_event_list.c \
	src/database/mysql.c \
	src/comms/new_mail.c \
	src/olc/oasis.c \
	src/olc/oasis_copy.c \
	src/olc/oasis_delete.c \
	src/olc/oasis_list.c \
	src/obj/objsave.c \
	src/obj/player_shop.c \
	src/olc/oedit.c \
	src/player/password.c \
	src/core/perfmon.c \
	src/wilderness/perlin.c \
	src/player/player_rename.c \
	src/player/player_rename.h \
	src/player/players.c \
	src/olc/prefedit.c \
	src/character/premadebuilds.c \
	src/net/msdp_json.c \
	src/net/msdp_json.h \
	src/net/protocol.c \
	src/magic/psionics.c \
	src/wilderness/spatial_core.c \
	src/wilderness/spatial_visual.c \
	src/wilderness/spatial_audio.c \
	src/wilderness/spatial_events.c \
	src/wilderness/spatial_events.h \
	src/mob/phenomenon_response.c \
	src/mob/phenomenon_response.h \
	src/wilderness/terrain_bridge.c \
	src/wilderness/region_hints.c \
	src/wilderness/narrative_weaver.c \
	src/net/onboarding.c \
	src/net/i3_client.c \
	src/net/i3_commands.c \
	src/net/i3_utils.c \
	src/olc/qedit.c \
	src/quest/quest.c \
	src/quest/quest_services.c \
	src/character/race.c \
	src/core/random.c \
	src/mob/random_names.c \
	src/player/rank.c \
	src/character/rewards.c \
	src/character/rol_feats.c \
	src/olc/redit.c \
	src/wilderness/resource_system.c \
	src/wilderness/harvest.c \
	src/wilderness/harvest.h \
	src/config/harvest_vnums.h \
	src/wilderness/resource_depletion.c \
	src/wilderness/resource_descriptions.c \
	src/wilderness/resource_regeneration.c \
	src/character/roleplay.c \
	src/vessels/routing.c \
	src/olc/sedit.c \
	src/olc/spec_menu.c \
	src/obj/shop.c \
	src/combat/spec_abilities.c \
	src/spec/spec_assign_table.c \
	src/spec/spec_binding.c \
	src/spec/spec_combat.c \
	src/spec/spec_context.c \
	src/spec/spec_cooldown.c \
	src/spec/spec_dispatch.c \
	src/spec/spec_effects.c \
	src/spec/spec_effective_binding.c \
	src/spec/spec_mobile_archetypes.c \
	src/spec/spec_mobiles.c \
	src/spec/spec_objects.c \
	src/spec/spec_phrase.c \
	src/spec/spec_rol_avernus.c \
	src/spec/spec_rol_combat.c \
	src/spec/spec_rol_darkhold.c \
	src/spec/spec_rol_deaths_head.c \
	src/spec/spec_rol_drow.c \
	src/spec/spec_rol_residual_mobile.c \
	src/spec/spec_rol_conversion.c \
	src/spec/spec_rol_lavatubes.c \
	src/spec/spec_rol_tarrasque.c \
	src/spec/spec_rol_utility_objects.c \
	src/spec/spec_rol_periodic_profiles.inc \
	src/spec/spec_rol_state_periodic_profiles.inc \
	src/spec/spec_rol_pilot.c \
	src/spec/spec_rol_totem.c \
	src/spec/spec_rooms.c \
	src/spec/spec_zone_abyss.c \
	src/spec/spec_zone_abyssal_vortex.c \
	src/spec/spec_zone_agrach_dyrr.c \
	src/spec/spec_zone_air_plane.c \
	src/spec/spec_zone_alarm_group.c \
	src/spec/spec_zone_bandit_castle.c \
	src/spec/spec_zone_battlemaze.c \
	src/spec/spec_zone_banshee.c \
	src/spec/spec_zone_celestial_leviathan.c \
	src/spec/spec_zone_crimson_flame.c \
	src/spec/spec_zone_earth_plane.c \
	src/spec/spec_zone_feybranche.c \
	src/spec/spec_zone_fire_giant.c \
	src/spec/spec_zone_flaming_tower.c \
	src/spec/spec_zone_hive_of_passion.c \
	src/spec/spec_zone_illithid_enclave.c \
	src/spec/spec_zone_jot.c \
	src/spec/spec_zone_kenjin_tower.c \
	src/spec/spec_zone_kings_castle.c \
	src/spec/spec_zone_kobold_caverns.c \
	src/spec/spec_zone_longsaddle.c \
	src/spec/spec_zone_mad_drow.c \
	src/spec/spec_zone_mere_of_dead_men.c \
	src/spec/spec_zone_menzoberranzan.c \
	src/spec/spec_zone_neverwinter.c \
	src/spec/spec_zone_orc_ruins.c \
	src/spec/spec_zone_prisoner.c \
	src/spec/spec_zone_quicksand.c \
	src/spec/spec_zone_secomber.c \
	src/spec/spec_zone_shadow_dragon.c \
	src/spec/spec_zone_shobalar.c \
	src/spec/spec_zone_ttf.c \
	src/spec/spec_zone_zusuk.c \
	src/spec/spec_registry.c \
	src/spec/spec_assign.c \
	src/spec/spec_assign_mobiles.c \
	src/spec/spec_assign_objects.c \
	src/spec/spec_assign_rooms.c \
	src/obj/spec_artifacts.c \
	src/magic/casting_visuals.c \
	src/magic/spell_parser.c \
	src/magic/spell_prep.c \
	src/magic/spell_lists.c \
	src/magic/spellbook_scroll.c \
	src/magic/spells.c \
	src/quest/staff_events.c \
	src/character/skill_lists.c \
	src/character/study.c \
	src/olc/tedit.c \
	src/character/templates.c \
	src/obj/trade.c \
	src/obj/vendor.c \
	src/character/vampire_cloak.c \
	src/vessels/transport.c \
	src/combat/traps_new.c \
	src/obj/treasure.c \
	src/obj/treasure_const.c \
	src/core/utils.c \
	src/vessels/vehicles.c \
	src/vessels/vehicles_commands.c \
	src/vessels/vehicles_transport.c \
	src/vessels/transport_unified.c \
	src/vessels/vessels.c \
	src/vessels/vessels_legacy.c \
	src/vessels/vessels_rol.c \
	src/vessels/vessels_moving_rooms.c \
	src/vessels/vessels_tactical.c \
	src/vessels/vessels_lookout.c \
	src/vessels/vessels_narrative.c \
	src/vessels/vessels_admin.c \
	src/vessels/vessels_balance.c \
	src/vessels/vessels_combat.c \
	src/vessels/vessels_crew.c \
	src/vessels/vessels_hazards.c \
	src/vessels/vessels_hunters.c \
	src/vessels/vessels_merchants.c \
	src/vessels/vessels_events.c \
	src/vessels/vessels_db.c \
	src/vessels/vessels_docking.c \
	src/vessels/vessels_edit.c \
	src/vessels/vessels_ownership.c \
	src/vessels/vessels_rooms.c \
	src/vessels/vessels_contracts.c \
	src/vessels/vessels_piracy.c \
	src/vessels/vessels_trade.c \
	src/vessels/vessels_upgrades.c \
	src/vessels/vessels_autopilot.c \
	src/core/weather.c \
	src/wilderness/wilderness.c \
	src/wilderness/wilderness_kb.c \
	src/olc/zedit.c \
	src/core/zmalloc.c

# Libraries to link
luminari_LDADD = @LIBS@ @CRYPTLIB@ @NETLIB@ @LIBEVENT_LIBS@ -lcrypt -lgd -lm -lmariadb -lcurl -lssl -lcrypto -lpthread -ljson-c


# Unit testing with CuTest and the bundled snprintf fallback
check_PROGRAMS = cutest bsd_snprintf_fallback_test
TESTS = bsd_snprintf_fallback_test

bsd_snprintf_fallback_test_SOURCES = src/core/bsd-snprintf.c
bsd_snprintf_fallback_test_CPPFLAGS = $(AM_CPPFLAGS) -DBROKEN_SNPRINTF -DTEST_SNPRINTF
bsd_snprintf_fallback_test_LDADD = -lm

# CuTest sources
cutest_SOURCES = \
	unittests/CuTest/test_asciimap_production.c \
	unittests/CuTest/test_syntax_check_boot.c \
	unittests/CuTest/test_web_onboarding.c \
	unittests/CuTest/test_race_equivalence.c \
	unittests/CuTest/test_i3_client_production.c \
	unittests/CuTest/test_msdp_production.c \
	unittests/CuTest/test_necromancer.c \
	unittests/CuTest/test_pet_persistence.c \
	unittests/CuTest/test_pet_policy.c \
	unittests/CuTest/test_pet_lifetime.c \
	unittests/CuTest/test_study_pet_menus.c \
	unittests/CuTest/CuTest.c \
	unittests/CuTest/test_bounds_checking.c \
	unittests/CuTest/test_help_sync.c \
	unittests/CuTest/test_artifacts.c \
	unittests/CuTest/test_artifact_integration.c \
	unittests/CuTest/test_player_rename.c \
	unittests/CuTest/test_ai_secrets.c \
	unittests/CuTest/test.helpers.c \
	unittests/CuTest/test.interpreter.c \
	unittests/CuTest/test_transport_production.c \
	unittests/CuTest/test_traps_production.c \
	unittests/CuTest/test_combat_production.c \
	unittests/CuTest/test_combat_messages.c \
	unittests/CuTest/test_perk_definitions.c \
	unittests/CuTest/test_wizard_set.c \
	unittests/CuTest/test_combat_encounters.c \
	unittests/CuTest/test_activity_manager.c \
	unittests/CuTest/test_thrown_weapons.c \
	unittests/CuTest/test_lists_production.c \
	unittests/CuTest/test_mob_autoroll.c \
	unittests/CuTest/test_copyover_timer.c \
	unittests/CuTest/test_reactor.c \
	unittests/CuTest/test_game_scheduler.c \
	unittests/CuTest/test_event_runtime.c \
	unittests/CuTest/test_door_events.c \
	unittests/CuTest/test_domain_events.c \
	unittests/CuTest/test_native_event_integration.c \
	unittests/CuTest/test_bardic_performance.c \
	unittests/CuTest/test_rol_feats.c \
	unittests/CuTest/test_racial_innate_feats.c \
	unittests/CuTest/test_four_arms.c \
	unittests/CuTest/test_who_command.c \
	unittests/CuTest/test_unassigned_spells.c \
	unittests/CuTest/test_spells_skills_production.c \
	unittests/CuTest/test_perfmon_production.c \
	unittests/CuTest/test_elf_build_id.c \
	unittests/CuTest/test_vessel_events.c \
	unittests/CuTest/test_vessel_tactical.c \
	unittests/CuTest/test_vessel_lookout.c \
	unittests/CuTest/test_vessel_narrative.c \
	unittests/CuTest/test_vessel_boarding.c \
	unittests/CuTest/test_database_persistence.c \
	unittests/CuTest/test_dg_scripts_production.c \
	unittests/CuTest/test_ashenport_welcome_trigger.c \
	unittests/CuTest/test_damage_trigger.c \
	unittests/CuTest/test_upstream_regressions.c \
	unittests/CuTest/test_world_loading_production.c \
	unittests/CuTest/test_spec_fixtures.c \
	unittests/CuTest/test_spec_authored_bindings.c \
	unittests/CuTest/test_spec_binding_round_trip.c \
	unittests/CuTest/test_spec_effective_binding.c \
	unittests/CuTest/test_spec_registry_persistence.c \
	unittests/CuTest/test_spec_registry_validation.c \
	unittests/CuTest/test_spec_owner_aware_olc.c \
	unittests/CuTest/test_spec_command_pulse.c \
	unittests/CuTest/test_spec_assign_table.c \
	unittests/CuTest/test_spec_dispatch.c \
	unittests/CuTest/test_spec_mechanics.c \
	unittests/CuTest/test_spec_typed_handlers.c \
	unittests/CuTest/test_spec_combat_secondary.c \
	unittests/CuTest/test_gameplay_e2e.c \
	unittests/CuTest/test_password.c \
	unittests/CuTest/test_rewards_production.c \
	unittests/CuTest/test_craft_training.c \
	unittests/CuTest/test_binary_formats.c \
	unittests/CuTest/test_binary_file_persistence.c \
	$(luminari_SOURCES)

# Generated by make-tests.sh at build time; never shipped in a distribution.
nodist_cutest_SOURCES = unittests/CuTest/AllTests.c unittests/CuTest/test_prototypes.h

cutest_CFLAGS = $(AM_CFLAGS) -DLUMINARI_CUTEST
cutest_LDADD = $(luminari_LDADD)

cutest_test_files = \
	unittests/CuTest/test_asciimap_production.c \
	unittests/CuTest/test_syntax_check_boot.c \
	unittests/CuTest/test_bounds_checking.c \
	unittests/CuTest/test_help_sync.c \
	unittests/CuTest/test_web_onboarding.c \
	unittests/CuTest/test_race_equivalence.c \
	unittests/CuTest/test_i3_client_production.c \
	unittests/CuTest/test_msdp_production.c \
	unittests/CuTest/test_necromancer.c \
	unittests/CuTest/test_pet_persistence.c \
	unittests/CuTest/test_pet_policy.c \
	unittests/CuTest/test_pet_lifetime.c \
	unittests/CuTest/test_study_pet_menus.c \
	unittests/CuTest/test_artifacts.c \
	unittests/CuTest/test_artifact_integration.c \
	unittests/CuTest/test_player_rename.c \
	unittests/CuTest/test_ai_secrets.c \
	unittests/CuTest/test.helpers.c \
	unittests/CuTest/test.interpreter.c \
	unittests/CuTest/test_transport_production.c \
	unittests/CuTest/test_traps_production.c \
	unittests/CuTest/test_combat_production.c \
	unittests/CuTest/test_combat_messages.c \
	unittests/CuTest/test_perk_definitions.c \
	unittests/CuTest/test_wizard_set.c \
	unittests/CuTest/test_combat_encounters.c \
	unittests/CuTest/test_activity_manager.c \
	unittests/CuTest/test_thrown_weapons.c \
	unittests/CuTest/test_lists_production.c \
	unittests/CuTest/test_mob_autoroll.c \
	unittests/CuTest/test_copyover_timer.c \
	unittests/CuTest/test_reactor.c \
	unittests/CuTest/test_game_scheduler.c \
	unittests/CuTest/test_event_runtime.c \
	unittests/CuTest/test_door_events.c \
	unittests/CuTest/test_domain_events.c \
	unittests/CuTest/test_native_event_integration.c \
	unittests/CuTest/test_bardic_performance.c \
	unittests/CuTest/test_rol_feats.c \
	unittests/CuTest/test_racial_innate_feats.c \
	unittests/CuTest/test_four_arms.c \
	unittests/CuTest/test_who_command.c \
	unittests/CuTest/test_unassigned_spells.c \
	unittests/CuTest/test_spells_skills_production.c \
	unittests/CuTest/test_perfmon_production.c \
	unittests/CuTest/test_elf_build_id.c \
	unittests/CuTest/test_vessel_events.c \
	unittests/CuTest/test_vessel_tactical.c \
	unittests/CuTest/test_vessel_lookout.c \
	unittests/CuTest/test_vessel_narrative.c \
	unittests/CuTest/test_vessel_boarding.c \
	unittests/CuTest/test_database_persistence.c \
	unittests/CuTest/test_dg_scripts_production.c \
	unittests/CuTest/test_ashenport_welcome_trigger.c \
	unittests/CuTest/test_damage_trigger.c \
	unittests/CuTest/test_upstream_regressions.c \
	unittests/CuTest/test_world_loading_production.c \
	unittests/CuTest/test_spec_fixtures.c \
	unittests/CuTest/test_spec_authored_bindings.c \
	unittests/CuTest/test_spec_binding_round_trip.c \
	unittests/CuTest/test_spec_effective_binding.c \
	unittests/CuTest/test_spec_registry_persistence.c \
	unittests/CuTest/test_spec_registry_validation.c \
	unittests/CuTest/test_spec_owner_aware_olc.c \
	unittests/CuTest/test_spec_command_pulse.c \
	unittests/CuTest/test_spec_assign_table.c \
	unittests/CuTest/test_spec_dispatch.c \
	unittests/CuTest/test_spec_mechanics.c \
	unittests/CuTest/test_spec_typed_handlers.c \
	unittests/CuTest/test_spec_combat_secondary.c \
	unittests/CuTest/test_gameplay_e2e.c \
	unittests/CuTest/test_password.c \
	unittests/CuTest/test_rewards_production.c \
	unittests/CuTest/test_craft_training.c \
	unittests/CuTest/test_binary_formats.c \
	unittests/CuTest/test_binary_file_persistence.c

# Generate AllTests.c before building cutest
unittests/CuTest/AllTests.c: $(cutest_test_files) unittests/CuTest/make-tests.sh
	unittests/CuTest/make-tests.sh $(cutest_test_files) > $@

# Prototypes for every Test function, included by CuTest.h in the root suite
unittests/CuTest/test_prototypes.h: $(cutest_test_files) unittests/CuTest/make-tests.sh
	unittests/CuTest/make-tests.sh --prototypes $(cutest_test_files) > $@

# BUILT_SOURCES only orders the all, check, and install targets; `make cutest`
# (the sanitizer, coverage, and memory-check jobs) must still wait for the
# generated prototypes that CuTest.h includes.
$(cutest_OBJECTS): unittests/CuTest/test_prototypes.h

BUILT_SOURCES = build_identity.h unittests/CuTest/AllTests.c unittests/CuTest/test_prototypes.h

# Clean up generated files
CLEANFILES = build_identity.h unittests/CuTest/AllTests.c unittests/CuTest/test_prototypes.h

.PHONY: FORCE_BUILD_IDENTITY
FORCE_BUILD_IDENTITY:

build_identity.h: FORCE_BUILD_IDENTITY scripts/deployment/generate_build_identity.sh
	$(AM_V_GEN)$(SHELL) $(abs_top_srcdir)/scripts/deployment/generate_build_identity.sh \
		$@ $(abs_top_srcdir)

# Direct parallel targets such as `make -j cutest` do not receive Automake's
# BUILT_SOURCES ordering.  Both variants of constants.o include this header.
src/core/constants.$(OBJEXT) src/core/cutest-constants.$(OBJEXT): build_identity.h

# GCC's analyzer needs more than 30 GiB for load_class_list, which registers
# every class in one function, so class.c is compiled without it. Owner: class
# registration in src/character/class.c. Expires when load_class_list is split
# into functions the analyzer finishes within a CI runner's memory; remove the
# matching exclusion in CMakeLists.txt at the same time.
analyzer_exclude = $(if $(filter -fanalyzer,$(MYFLAGS)),-fno-analyzer)
src/character/class.$(OBJEXT) src/character/cutest-class.$(OBJEXT): CFLAGS += $(analyzer_exclude)

# Stage the server away from the mutable launch alias. The install hook below
# promotes it into an immutable build-ID release and updates bin/luminari.
bindir = $(top_builddir)/bin/.luminari-install

# After installation, remove the executable from the source directory
install-exec-hook:
	$(SHELL) $(abs_top_srcdir)/scripts/deployment/install_versioned_binary.sh \
		$(bindir)/luminari $(top_builddir)/bin
	$(SHELL) $(abs_top_srcdir)/scripts/provision_artifacts.sh
	rm -f $(bindir)/luminari
	rmdir $(bindir)
	rm -f $(bin_PROGRAMS)

# Ensure constants.c is rebuilt when other sources change
src/core/constants.c: $(filter-out src/core/constants.c,$(luminari_SOURCES))
	touch $@

# Standalone Python world-data tooling. Keep this list synchronized with
# WORLD_TOOL_SOURCES in CMakeLists.txt.
world_tool_sources = \
	scripts/world/install_pet_constructs.py \
	data/pet-constructs/196.mob \
	data/pet-planar-allies/197.mob \
	data/pet-undead/198.mob \
	data/pet-lycanthropes/195.mob \
	data/pet-lycanthropes/195.zon \
	data/pet-illusions/199.mob \
	data/pet-illusions/199.zon \
	data/pet-undead/198.zon \
	data/pet-undead/README.md \
	data/pet-planar-allies/197.zon \
	data/pet-planar-allies/README.md \
	data/pet-constructs/README.md \
	data/pet-constructs/196.zon \
	scripts/world/generate_rol_periodic_profiles.py \
	scripts/world/generate_rol_state_periodic_profiles.py \
	scripts/world/wtool.py \
	scripts/world/wtool_constants.json \
	scripts/world/rol_conversion_policy.json \
	scripts/world/rol_source_race_registry.json \
	scripts/world/wtool_lib/__init__.py \
	scripts/world/wtool_lib/cli.py \
	scripts/world/wtool_lib/config.py \
	scripts/world/wtool_lib/constants.py \
	scripts/world/wtool_lib/docs_check.py \
	scripts/world/wtool_lib/flags.py \
	scripts/world/wtool_lib/hlquests.py \
	scripts/world/wtool_lib/indexes.py \
	scripts/world/wtool_lib/lookup.py \
	scripts/world/wtool_lib/models.py \
	scripts/world/wtool_lib/mobiles.py \
	scripts/world/wtool_lib/objects.py \
	scripts/world/wtool_lib/parsing.py \
	scripts/world/wtool_lib/quests.py \
	scripts/world/wtool_lib/reporting.py \
	scripts/world/wtool_lib/rol_baseline.py \
	scripts/world/wtool_lib/rol_capability_audit.py \
	scripts/world/wtool_lib/rol_discovery.py \
	scripts/world/wtool_lib/rol_graph.py \
	scripts/world/wtool_lib/rol_inventory.py \
	scripts/world/wtool_lib/rol_json.py \
	scripts/world/wtool_lib/rol_mob_calculator.py \
	scripts/world/wtool_lib/rol_mobile_identity.py \
	scripts/world/wtool_lib/rol_identity.py \
	scripts/world/wtool_lib/rol_persistence.py \
	scripts/world/wtool_lib/rol_persistence_check.py \
	scripts/world/wtool_lib/rol_planner.py \
	scripts/world/wtool_lib/rol_phase7.py \
	scripts/world/wtool_lib/rol_phase8.py \
	scripts/world/wtool_lib/rol_pilot.py \
	scripts/world/wtool_lib/rol_pilot_build.py \
	scripts/world/wtool_lib/rol_periodic_profiles.py \
	scripts/world/wtool_lib/rol_skeleton.py \
	scripts/world/wtool_lib/rol_state_periodic_profiles.py \
	scripts/world/wtool_lib/rol_soc.py \
	scripts/world/wtool_lib/rol_special.py \
	scripts/world/wtool_lib/rol_conversion_types.py \
	scripts/world/wtool_lib/rol_source_common.py \
	scripts/world/wtool_lib/rol_transform_common.py \
	scripts/world/wtool_lib/rol_mobiles.py \
	scripts/world/wtool_lib/rol_objects.py \
	scripts/world/wtool_lib/rol_armor_mapping.py \
	scripts/world/wtool_lib/rol_armor_overrides.json \
	scripts/world/wtool_lib/rol_weapon_overrides.json \
	scripts/world/wtool_lib/rol_rooms.py \
	scripts/world/wtool_lib/rol_zones.py \
	scripts/world/wtool_lib/rol_shops.py \
	scripts/world/wtool_lib/rol_quests.py \
	scripts/world/wtool_lib/rol_source.py \
	scripts/world/wtool_lib/rol_transform.py \
	scripts/world/wtool_lib/rooms.py \
	scripts/world/wtool_lib/semantics.py \
	scripts/world/wtool_lib/shops.py \
	scripts/world/wtool_lib/source.py \
	scripts/world/wtool_lib/spec_registry.py \
	scripts/world/wtool_lib/triggers.py \
	scripts/world/wtool_lib/world.py \
	scripts/world/wtool_lib/zones.py \
	scripts/world/tests/__init__.py \
	scripts/world/tests/test_cli.py \
	scripts/world/tests/test_constants.py \
	scripts/world/tests/test_docs_check.py \
	scripts/world/tests/test_graph.py \
	scripts/world/tests/test_hlquests.py \
	scripts/world/tests/test_indexes.py \
	scripts/world/tests/test_lookup.py \
	scripts/world/tests/test_mobiles.py \
	scripts/world/tests/test_objects.py \
	scripts/world/tests/test_quests.py \
	scripts/world/tests/test_reporting.py \
	scripts/world/tests/test_rol_baseline.py \
	scripts/world/tests/rol_reference.py \
	scripts/world/tests/test_rol_capability_audit.py \
	scripts/world/tests/test_rol_discovery.py \
	scripts/world/tests/test_rol_graph.py \
	scripts/world/tests/test_rol_inventory.py \
	scripts/world/tests/test_rol_mob_calculator.py \
	scripts/world/tests/test_rol_mobile_identity.py \
	scripts/world/tests/test_rol_identity.py \
	scripts/world/tests/test_rol_persistence_check.py \
	scripts/world/tests/test_rol_planner.py \
	scripts/world/tests/test_rol_phase7.py \
	scripts/world/tests/test_rol_phase8.py \
	scripts/world/tests/test_rol_pilot.py \
	scripts/world/tests/test_rol_pilot_build.py \
	scripts/world/tests/test_rol_periodic_profiles.py \
	scripts/world/tests/test_rol_skeleton.py \
	scripts/world/tests/test_rol_state_periodic_profiles.py \
	scripts/world/tests/test_rol_source.py \
	scripts/world/tests/test_rol_transform.py \
	scripts/world/tests/test_rooms.py \
	scripts/world/tests/test_semantics.py \
	scripts/world/tests/test_shops.py \
	scripts/world/tests/test_source.py \
	scripts/world/tests/test_triggers.py \
	scripts/world/tests/test_wrapper.py \
	scripts/world/tests/test_zones.py \
	scripts/world/tests/fixtures/rol_inventory/malformed/areas/AREA \
	scripts/world/tests/fixtures/rol_inventory/malformed/areas/AREA.mobobj \
	scripts/world/tests/fixtures/rol_inventory/malformed/areas/QUEST \
	scripts/world/tests/fixtures/rol_inventory/malformed/areas/SHOP \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/AREA \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/AREA.mobobj \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/QUEST \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/SHOP \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/mob/alpha.mob \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/mob/disabled.mob \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/mob/orphan.mob \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/obj/alpha.obj \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/obj/companion.obj \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/obj/disabled.obj \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/obj/orphan.obj \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/qst/alpha.qst \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/qst/companion.qst \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/qst/disabled.qst \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/shp/alpha.shp \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/soc/alpha.soc \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/wld/alpha.wld \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/wld/disabled.wld \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/wld/orphan.wld \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/wld/split.wld \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/zon/alpha.zon \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/zon/disabled.zon \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/zon/orphan.zon \
	scripts/world/tests/fixtures/rol_inventory/valid/areas/zon/split.zon \
	scripts/world/tests/fixtures/phase1/valid/100.zon \
	scripts/world/tests/fixtures/phase1/valid/100.wld \
	scripts/world/tests/fixtures/phase1/broken/diagonal.wld \
	scripts/world/tests/fixtures/phase1/broken/header-truncation.zon \
	scripts/world/tests/fixtures/phase1/broken/invalid-directions.wld \
	scripts/world/tests/fixtures/phase1/broken/invalid-directions.zon \
	scripts/world/tests/fixtures/phase1/broken/moving-limit.wld \
	scripts/world/tests/fixtures/phase1/broken/out-of-order.wld \
	scripts/world/tests/fixtures/phase1/broken/prescan.zon \
	scripts/world/tests/fixtures/phase1/broken/reset-traps.zon \
	scripts/world/tests/fixtures/phase1/broken/room-structure.wld \
	scripts/world/tests/fixtures/phase2/complete/hlq/100.hlq \
	scripts/world/tests/fixtures/phase2/complete/hlq/index \
	scripts/world/tests/fixtures/phase2/complete/hlq/index.mini \
	scripts/world/tests/fixtures/phase2/complete/mob/100.mob \
	scripts/world/tests/fixtures/phase2/complete/mob/index \
	scripts/world/tests/fixtures/phase2/complete/mob/index.mini \
	scripts/world/tests/fixtures/phase2/complete/obj/100.obj \
	scripts/world/tests/fixtures/phase2/complete/obj/index \
	scripts/world/tests/fixtures/phase2/complete/obj/index.mini \
	scripts/world/tests/fixtures/phase2/complete/qst/100.qst \
	scripts/world/tests/fixtures/phase2/complete/qst/index \
	scripts/world/tests/fixtures/phase2/complete/qst/index.mini \
	scripts/world/tests/fixtures/phase2/complete/shp/100.shp \
	scripts/world/tests/fixtures/phase2/complete/shp/index \
	scripts/world/tests/fixtures/phase2/complete/shp/index.mini \
	scripts/world/tests/fixtures/phase2/complete/shp/legacy.shp \
	scripts/world/tests/fixtures/phase2/complete/trg/100.trg \
	scripts/world/tests/fixtures/phase2/complete/trg/index \
	scripts/world/tests/fixtures/phase2/complete/trg/index.mini \
	scripts/world/tests/fixtures/phase2/complete/wld/100.wld \
	scripts/world/tests/fixtures/phase2/complete/wld/index \
	scripts/world/tests/fixtures/phase2/complete/wld/index.mini \
	scripts/world/tests/fixtures/phase2/complete/zon/100.zon \
	scripts/world/tests/fixtures/phase2/complete/zon/index \
	scripts/world/tests/fixtures/phase2/complete/zon/index.mini \
	scripts/world/tests/fixtures/phase2/contracts/hlq/canonical.hlq \
	scripts/world/tests/fixtures/phase2/contracts/lookup/existing-six.json \
	scripts/world/tests/fixtures/phase2/contracts/qst/canonical.qst \
	scripts/world/tests/fixtures/phase2/contracts/qst/legacy.qst

# Runtime and documentation inputs needed by the world-tool gates. Keep this
# list synchronized with WORLD_TOOL_SUPPORT_FILES in CMakeLists.txt.
world_tool_support_files = \
	lib/world/validate-zone.sh \
	lib/world/minimal/0.mob \
	lib/world/minimal/0.obj \
	lib/world/minimal/0.wld \
	lib/world/minimal/0.zon \
	lib/world/minimal/16.mob \
	scripts/development/generate-web-guides.sh \
	docs/guides/OLC_SpecProcs.md \
	docs/systems/OLC_ONLINE_CREATION_SYSTEM.md \
	docs/utilities/WORLD_VALIDATOR_CLI.md \
	docs/web/assets/pandoc-template.html \
	docs/web/guides/mob_flags.html \
	docs/web/guides/oedit.html \
	docs/web/guides/room_flags.html \
	docs/world/STARTER_AREA.md \
	docs/world_game-data/BUILDER_QUICKSTART.md \
	docs/world_game-data/CRAFTING_SYSTEM_NOTES.md \
	docs/world_game-data/MOB_FLAGS.md \
	docs/world_game-data/HLQUEST_FILE_FORMAT.md \
	docs/world_game-data/OEDIT_GUIDE.md \
	docs/world_game-data/QUEST_FILE_FORMAT.md \
	docs/world_game-data/ROOM_FLAGS.md \
	docs/world_game-data/SHOP_FILE_FORMAT.md \
	docs/world_game-data/ZONE_FILE_FORMAT.md \
	docs/world_game-data/builder_manual.md \
	docs/world_game-data/gear_guide.md \
	docs/world_game-data/wilderness_system.md

help_sync_sources = \
	.agents/skills/help-sync/SKILL.md \
	.agents/skills/help-sync/agents/openai.yaml \
	scripts/help-sync/catalog.py \
	scripts/help-sync/endpoint.py \
	scripts/help-sync/help_sync.py \
	scripts/help-sync/requirements.txt \
	scripts/help-sync/tests/test_catalog.py \
	scripts/help-sync/tests/test_endpoint.py \
	scripts/help-sync/tests/test_endpoint_integration.py \
	sql/components/help_sync_schema.sql \
	sql/components/verify_help_sync_schema.sql

# Extra files to distribute
EXTRA_DIST = \
	scripts/development/generate_msp_sounds.py \
	lib/sounds/README.md \
	lib/sounds/luminari-test.wav \
	lib/sounds/luminari-door-open.wav \
	sql/components/help_screen_reader_msp.sql \
	$(world_tool_sources) \
	$(world_tool_support_files) \
	$(help_sync_sources) \
	lib/world/artifacts/1699.mob \
	lib/world/artifacts/1699.obj \
	lib/world/artifacts/1699.wld \
	lib/world/artifacts/1699.zon \
	lib/world/artifacts/artifacts.hlp \
	unittests/README_unittests.md \
	unittests/CuTest/Makefile \
	unittests/CuTest/README.txt \
	unittests/CuTest/bench_protocol_parser.c \
	unittests/CuTest/fuzz_protocol_parser.c \
	unittests/CuTest/fuzz_corpus/plain_command.txt \
	unittests/CuTest/fuzz_corpus/gmcp_like.txt \
	unittests/CuTest/fuzz_corpus/output_unicode.txt \
	unittests/CuTest/binary_format_fixtures.h \
	unittests/CuTest/fuzz_binary_formats.c \
	unittests/CuTest/fuzz_corpus_binary_formats/board_current.txt \
	unittests/CuTest/fuzz_corpus_binary_formats/board_legacy.txt \
	unittests/CuTest/fuzz_corpus_binary_formats/house_current.txt \
	unittests/CuTest/fuzz_corpus_binary_formats/house_legacy.txt \
	unittests/CuTest/fuzz_corpus_binary_formats/last_legacy.txt \
	unittests/CuTest/fixtures/spec_world_inventory/mob/inventory.mob \
	unittests/CuTest/fixtures/spec_world_inventory/obj/inventory.obj \
	unittests/CuTest/fixtures/spec_world_inventory/wld/inventory.wld \
	unittests/CuTest/fixtures/ashenport_welcome/103003.trg \
	unittests/CuTest/make-tests.sh \
	unittests/CuTest/test_protocol_parser.c \
	autorun-watchdog.sh \
	autorun.sh \
	scripts/README_scripts.md \
	scripts/autorun/autorun-watchdog.sh \
	scripts/autorun/autorun.sh \
	scripts/autorun/test_autorun_supervision.sh \
	scripts/backup.sh \
	scripts/cbuild.sh \
	scripts/character-rename/test_character_rename_schema.sh \
	scripts/character-rename/test_character_rename_static.sh \
	scripts/copyover/copyover_watchdog.sh \
	scripts/copyover/enhanced_copyover_diagnostic.sh \
	scripts/ci/check_clean_archive.sh \
	scripts/ci/prepare_test_runtime.sh \
	scripts/ci/test_clean_archive.py \
	scripts/ci/test_cutest_runner.py \
	scripts/ci/check_sql_interpolation.py \
	scripts/ci/sql_interpolation_baseline.txt \
	scripts/ci/check_native_struct_io.py \
	scripts/ci/check_compiler.sh \
	scripts/ci/check_configure_probes.sh \
	scripts/ci/check_warning_budget.py \
	scripts/ci/warning_budget_gcc-16.txt \
	scripts/ci/warning_budget_clang-22.txt \
	scripts/debugging/debug_game.sh \
	scripts/debugging/verify_core_capture.sh \
	scripts/debugging/vgrind.sh \
	scripts/deploy.sh \
	scripts/deployment/deploy.sh \
	scripts/deployment/generate_build_identity.sh \
	scripts/deployment/install_versioned_binary.sh \
	scripts/deployment/move_bin.sh \
	scripts/deployment/production_profile.sh \
	scripts/deployment/setup.sh \
	scripts/deployment/test_production_profile.sh \
	scripts/deployment/test_versioned_binary_install.sh \
	scripts/deployment/verify_hardened_binary.sh \
	scripts/development/check_local_port_allocations.sh \
	scripts/development/dev_create_test_character.sh \
	scripts/development/dev_kohdee_login_smoke.sh \
	scripts/events/run_event_core_performance_gate.py \
	scripts/events/test_demand_driven_architecture.sh \
	scripts/events/test_event_core_performance_gate.py \
	scripts/events/test_legacy_event_admission.sh \
	scripts/events/test_native_event_architecture.sh \
	scripts/events/test_pubsub_retirement.sh \
	scripts/mariadb/setup_mariadb_sudoers.sh \
	scripts/mariadb/start_mariadb.sh \
	scripts/move_bin.sh \
	scripts/operations/healthcheck.sh \
	scripts/operations/test_healthcheck.sh \
	scripts/permissions/check_permissions.sh \
	scripts/permissions/fix_permissions.sh \
	scripts/process-memory/monitor_process_memory.sh \
	scripts/process-memory/sample_process_memory_details.sh \
	scripts/process-memory/test_monitor_process_memory.sh \
	scripts/process-memory/test_process_memory_details.sh \
	scripts/provision_artifacts.sh \
	scripts/setup.sh \
	scripts/test_background_help_entries.sh \
	scripts/vessels/analyze_vessel_memory_samples.sh \
	scripts/vessels/provision_vessel_campaign.sh \
	scripts/vessels/provision_vessel_derelict.sh \
	scripts/vessels/provision_vessel_frontier.sh \
	scripts/vessels/provision_vessel_harbor.sh \
	scripts/vessels/run_vessel_ferry_soak.sh \
	scripts/vessels/run_vessel_scale_benchmark.sh \
	scripts/vessels/test_vessel_boarding_in_game.sh \
	scripts/vessels/test_vessel_derelict_in_game.sh \
	scripts/vessels/test_vessel_events_in_game.sh \
	scripts/vessels/test_vessel_ferry_soak_preflight.sh \
	scripts/vessels/test_vessel_hunter_in_game.sh \
	scripts/vessels/test_vessel_lookout_in_game.sh \
	scripts/vessels/test_vessel_memory_analyzer.sh \
	scripts/vessels/test_vessel_merchant_in_game.sh \
	scripts/vessels/test_vessel_narrative_in_game.sh \
	scripts/vessels/test_vessel_scale_benchmark_parsers.sh \
	scripts/vessels/test_vessel_tactical_in_game.sh \
	scripts/world/populate_zone8000_from_south_of_waterdeep.py \
	scripts/world/populate_zone9000_from_savage_frontier.py \
	docs/product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md \
	docs/deployment/VESSEL_SCHEMA_DEPLOYMENT.md \
	docs/systems/VESSEL_SYSTEM.md \
	docs/testing/LOCAL_DEV_LOGIN_QUICK_GUIDE.md \
	docs/testing/VESSEL_BENCHMARKS.md \
	docs/testing/VESSEL_SYSTEM_TESTING.md \
	lib/world/artifacts/1699.mob \
	lib/world/artifacts/1699.obj \
	lib/world/artifacts/1699.wld \
	lib/world/artifacts/1699.zon \
	lib/world/artifacts/artifacts.hlp \
	lib/world/vessel_harbor/10000.wld \
	lib/world/vessel_harbor/700.mob \
	lib/world/vessel_harbor/700.trg \
	lib/world/vessel_harbor/700.zon \
	sql/components/character_rename_transactional_schema.sql \
	sql/components/help_arcane_mark_entries.sql \
	sql/components/help_nature_survival_entries.sql \
	sql/components/help_buff_sequence_entry.sql \
	sql/components/help_counterspell_readiness.sql \
	sql/components/help_defensive_casting_clock.sql \
	sql/components/help_bleeding_critical_clock.sql \
	sql/components/help_billowing_cloud_exposure.sql \
	sql/components/help_staff_event_agenda_entry.sql \
	sql/components/help_command_sweep_entries.sql \
	sql/components/help_craft_training_entries.sql \
	sql/components/help_pet_entries.sql \
	sql/components/help_dg_damage_trigger.sql \
	sql/components/help_rol_feat_entries.sql \
	sql/components/help_other_racial_innate_entries.sql \
	sql/components/help_rol_player_kits.sql \
	sql/components/help_race_half_illithid_entries.sql \
	sql/components/help_race_half_ogre_entries.sql \
	sql/components/help_race_myconid_entries.sql \
	sql/components/help_race_wemic_entries.sql \
	sql/components/help_race_yuan_ti_entries.sql \
	sql/components/help_semantic_combat_entries.sql \
	sql/components/help_trelux_equipment_entries.sql \
	sql/components/help_specproc_entries.sql \
	sql/components/help_thrown_weapons_entries.sql \
	sql/components/help_vessel_entries.sql \
	sql/components/help_character_backgrounds.sql \
	sql/components/help_map_shop_markers.sql \
	sql/components/help_necromancer_entries.sql \
	sql/components/help_rol_object_trap_entries.sql \
	sql/components/help_worship_entries.sql \
	sql/components/ci_schema_manifest.txt \
	sql/components/test_vessels_integrity.sql \
	sql/components/verify_help_arcane_mark_entries.sql \
	sql/components/verify_help_command_sweep_entries.sql \
	sql/components/verify_help_dg_damage_trigger.sql \
	sql/components/verify_help_necromancer_entries.sql \
	sql/components/verify_help_rol_feat_entries.sql \
	sql/components/verify_help_rol_player_kits.sql \
	sql/components/verify_help_rol_object_trap_entries.sql \
	sql/components/verify_help_race_half_illithid_entries.sql \
	sql/components/verify_help_race_half_ogre_entries.sql \
	sql/components/verify_help_race_myconid_entries.sql \
	sql/components/verify_help_race_wemic_entries.sql \
	sql/components/verify_help_race_yuan_ti_entries.sql \
	sql/components/verify_help_semantic_combat_entries.sql \
	sql/components/verify_help_trelux_equipment_entries.sql \
	sql/components/verify_help_specproc_entries.sql \
	sql/components/verify_help_thrown_weapons_entries.sql \
	sql/components/verify_help_vessel_entries.sql \
	sql/components/verify_help_worship_entries.sql \
	sql/components/verify_vessels_schema.sql \
	sql/components/verify_vessels_phase4.sql \
	sql/components/verify_vessels_phase6.sql \
	sql/components/verify_vessels_phase7.sql \
	sql/components/verify_vessels_phase8.sql \
	sql/components/verify_vessels_phase9.sql \
	sql/components/verify_vessels_phase10.sql \
	sql/components/verify_vessels_phase11.sql \
	sql/components/verify_vessels_phase12.sql \
	sql/components/verify_vessels_phase13.sql \
	sql/components/verify_vessels_phase14.sql \
	sql/components/verify_vessels_phase15.sql \
	sql/components/verify_vessels_phase16.sql \
	sql/components/verify_vessels_phase17.sql \
	sql/components/verify_vessels_narrative_content.sql \
	sql/components/vessels_harbor_sandbox.sql \
	sql/components/vessels_narrative_content.sql \
	sql/components/vessels_narrative_content_rollback.sql \
	sql/components/vessels_phase2_rollback.sql \
	sql/components/vessels_phase2_schema.sql \
	sql/components/vessels_phase4_rollback.sql \
	sql/components/vessels_phase4_schema.sql \
	sql/components/vessels_phase6_rollback.sql \
	sql/components/vessels_phase6_schema.sql \
	sql/components/vessels_phase7_rollback.sql \
	sql/components/vessels_phase7_schema.sql \
	sql/components/vessels_phase8_rollback.sql \
	sql/components/vessels_phase8_schema.sql \
	sql/components/vessels_phase9_rollback.sql \
	sql/components/vessels_phase9_schema.sql \
	sql/components/vessels_phase10_rollback.sql \
	sql/components/vessels_phase10_schema.sql \
	sql/components/vessels_phase11_rollback.sql \
	sql/components/vessels_phase11_schema.sql \
	sql/components/vessels_phase12_rollback.sql \
	sql/components/vessels_phase12_schema.sql \
	sql/components/vessels_phase13_rollback.sql \
	sql/components/vessels_phase13_schema.sql \
	sql/components/vessels_phase14_rollback.sql \
	sql/components/vessels_phase14_schema.sql \
	sql/components/vessels_phase15_rollback.sql \
	sql/components/vessels_phase15_schema.sql \
	sql/components/vessels_phase16_rollback.sql \
	sql/components/vessels_phase16_schema.sql \
	sql/components/vessels_phase17_rollback.sql \
	sql/components/vessels_phase17_schema.sql \
	sql/master_schema.sql \
	README.md

# Fail when the Autotools and CMake source manifests drift apart.
check-build-parity:
	cd "$(abs_top_srcdir)" && python3 scripts/ci/check_build_parity.py

test-cutest-runner:
	cd "$(abs_top_srcdir)" && CC="$(CC)" python3 scripts/ci/test_cutest_runner.py

test-clean-archive-runtime:
	cd "$(abs_top_srcdir)" && python3 scripts/ci/test_clean_archive.py -v

# Configure, build, and test an untouched git archive through both build systems.
distcheck-archive:
	"$(abs_top_srcdir)/scripts/ci/check_clean_archive.sh"

# Run unit tests
test: run-cutest test-cutest-runner check-build-parity test-clean-archive-runtime test-help-sync \
	test-autorun-supervision test-versioned-binary-install \
	test-binary-name-static test-healthcheck test-sql-interpolation test-production-profile \
	test-warning-budget test-background-help-entries test-demand-driven-architecture test-event-admission \
	test-event-core-performance-gate test-native-event-architecture test-pubsub-retirement \
	test-vessel-tooling test-header-self-containment test-clang-tidy-gate test-baseline-ratchet \
	test-native-struct-io

# Build check programs once before running the suite, alongside the shell gates.
# An inherited CUTEST_FILTER must not narrow the authoritative run.
run-cutest: check
	CUTEST_FILTER= LUMINARI_TEST_ROOT="$(abs_top_srcdir)" \
		LUMINARI_TEST_SPEC_WORLD_ROOT="$(abs_top_srcdir)/unittests/CuTest/fixtures/spec_world_inventory" \
		./cutest

.PHONY: test test-cutest-runner run-cutest test-process-memory check-build-parity \
	test-clean-archive-runtime distcheck-archive check-world-docs \
	rol-mob-calculator test-all test-world-tools test-help-sync \
	test-help-sync-integration test-protocol test-protocol-fuzz test-character-rename-static \
	test-character-rename-schema test-background-help-entries test-autorun-supervision \
	test-versioned-binary-install test-binary-name-static test-healthcheck \
	test-sql-interpolation test-production-profile test-warning-budget \
	test-demand-driven-architecture test-event-admission test-native-event-architecture \
	test-event-core-performance-gate test-pubsub-retirement \
	test-vessel-tooling test-header-self-containment test-clang-tidy-gate test-baseline-ratchet \
	test-native-struct-io

check-world-docs:
	cd "$(abs_top_srcdir)" && python3 scripts/world/wtool.py constants sync --check
	cd "$(abs_top_srcdir)" && python3 scripts/world/wtool.py docs --check

# Check recurses into util/. Share its calculator prerequisite with world tools
# before starting that recursive make, so the two paths cannot compile it at once.
check: rol-mob-calculator

rol-mob-calculator:
	$(MAKE) -C "$(abs_top_srcdir)/util" rol_mob_calculator

test-world-tools: check-world-docs rol-mob-calculator
	cd "$(abs_top_srcdir)" && python3 -m unittest discover -s scripts/world/tests -t scripts/world -v
	"$(abs_top_srcdir)/lib/world/validate-zone.sh" 100 \
		--world-root "$(abs_top_srcdir)/scripts/world/tests/fixtures/phase2/complete" \
		--json >/dev/null

test-help-sync:
	cd "$(abs_top_srcdir)" && python3 -m unittest discover \
		-s scripts/help-sync/tests -p 'test_*.py' -v

test-help-sync-integration:
	cd "$(abs_top_srcdir)" && LUMINARI_HELP_SYNC_INTEGRATION=1 \
		python3 -m unittest scripts/help-sync/tests/test_endpoint_integration.py -v

test-protocol:
	$(MAKE) -C "$(abs_top_srcdir)/unittests/CuTest" protocol-parser

test-protocol-fuzz:
	$(MAKE) -C "$(abs_top_srcdir)/unittests/CuTest" protocol-fuzz

test-character-rename-static:
	./scripts/character-rename/test_character_rename_static.sh

test-character-rename-schema:
	./scripts/character-rename/test_character_rename_schema.sh

test-background-help-entries:
	./scripts/test_background_help_entries.sh

test-demand-driven-architecture:
	./scripts/events/test_demand_driven_architecture.sh

test-event-core-performance-gate:
	cd "$(abs_top_srcdir)" && python3 -m unittest scripts/events/test_event_core_performance_gate.py -v

test-event-admission:
	./scripts/events/test_legacy_event_admission.sh

test-native-event-architecture:
	CC="$(CC)" ./scripts/events/test_native_event_architecture.sh

test-pubsub-retirement:
	./scripts/events/test_pubsub_retirement.sh

test-autorun-supervision:
	./scripts/autorun/test_autorun_supervision.sh

test-versioned-binary-install:
	./scripts/deployment/test_versioned_binary_install.sh

test-binary-name-static:
	$(SHELL) ./scripts/deployment/test_binary_name_static.sh

test-production-profile:
	CC="$(CC)" ./scripts/deployment/test_production_profile.sh

test-healthcheck:
	./scripts/operations/test_healthcheck.sh

# The migration-tier warning ratchet's own checks; the budget comparison
# itself needs a pinned compiler and runs in CI (issue #86).
test-warning-budget:
	cd "$(abs_top_srcdir)" && python3 scripts/ci/check_warning_budget.py --self-test
	cd "$(abs_top_srcdir)" && CC="$(CC)" scripts/ci/check_compiler.sh --cc "$(CC)" >/dev/null

# Blocks files read or written as native C structures (issue #95).
test-native-struct-io:
	cd "$(abs_top_srcdir)" && python3 scripts/ci/check_native_struct_io.py --self-test
	cd "$(abs_top_srcdir)" && python3 scripts/ci/check_native_struct_io.py

# Blocks new formatted SQL with data values (issue #98 ratchet).
test-sql-interpolation:
	cd "$(abs_top_srcdir)" && python3 scripts/ci/check_sql_interpolation.py --self-test
	cd "$(abs_top_srcdir)" && python3 scripts/ci/check_sql_interpolation.py

# Every header under src/ compiles on its own, apart from the shrinking baseline.
test-header-self-containment:
	cd "$(abs_top_srcdir)" && python3 scripts/ci/check_header_self_containment.py \
		--cc "$(CC)" --build-root "$(abs_top_builddir)"

test-clang-tidy-gate:
	cd "$(abs_top_srcdir)" && python3 scripts/ci/check_clang_tidy.py --self-test

# The comparison with the diff base runs in Code Quality, which knows the base.
test-baseline-ratchet:
	cd "$(abs_top_srcdir)" && python3 scripts/ci/check_baseline_ratchet.py --self-test

test-process-memory:
	./scripts/process-memory/test_process_memory_details.sh
	./scripts/process-memory/test_monitor_process_memory.sh

test-vessel-tooling:
	./scripts/vessels/test_vessel_ferry_soak_preflight.sh
	./scripts/vessels/test_vessel_memory_analyzer.sh
	./scripts/process-memory/test_process_memory_details.sh
	./scripts/process-memory/test_monitor_process_memory.sh
	./scripts/vessels/test_vessel_scale_benchmark_parsers.sh

# Authoritative full local test entry point. The final install both verifies the
# install path and removes the root-level luminari artifact created by make test.
test-all: test test-world-tools test-protocol test-character-rename-static \
	test-character-rename-schema
	$(MAKE) install

# Additional cleanup for distclean
# Removes autotools-generated files and build artifacts
distclean-local:
	rm -f Makefile.in install-sh gdb.txt depcomp compile missing
	rm -f configure~ configure.ac~ conf.h.in~ conf.h.in aclocal.m4
	rm -f config.status config.log configure
	rm -rf autom4te.cache/
	rm -rf src/.deps/ src/.dirstamp src/stamp-h1
	rm -f stamp-h1 .dirstamp
	rm -rf util/.deps/ util/.dirstamp
	rm -f config.cache

# Custom clean target that removes build files but keeps Makefile and config.h
# KEEPS: Everything needed to avoid triggering autoreconf during make clean
# The persistent .autorun.lock inode must never be unlinked by cleanup.
scrub: clean
	rm -f gdb.txt valgrind.log WORK.md CODEBASE_SCOPE.md
	rm -f configure~ configure.ac~ conf.h.in~
	rm -f config.log
	rm -rf autom4te.cache/
	rm -f log/*.log
	rm -f *.orig *.rej *.patch
	rm -f .killscript .fastboot pause
	rm -f .websocket_policy.pid .flash_policy.pid
	rm -f lib/copyover.dat
	rm -f core core.* vgcore.*
	rm -f lib/core lib/core.* lib/vgcore.*
	rm -f *~ *.swp *.swo
	rm -f src/*/*~ src/*/*.swp src/*/*.swo
	rm -f *.tmp *.temp
	rm -f test_*.out test_*.log
