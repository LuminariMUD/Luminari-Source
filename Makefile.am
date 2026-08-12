# LuminariMUD Makefile.am - for use with automake
# Based on the original Makefile.in structure

# Set up subdirectories
SUBDIRS = util

# Additional compiler flags
# Revert to project defaults: do NOT force global feature defines here.
AM_CFLAGS = @CSTD_CFLAGS@ @MYFLAGS@


# Main program
bin_PROGRAMS = circle

# Source files for the main program
circle_SOURCES = \
	src/account.c \
	src/act.comm.c \
	src/act.comm.do_spec_comm.c \
	src/act.informative.c \
	src/obj/act.item.c \
	src/movement/movement.c \
	src/movement/movement_validation.c \
	src/movement/movement_cost.c \
	src/movement/movement_position.c \
	src/movement/movement_doors.c \
	src/movement/movement_falling.c \
	src/movement/movement_messages.c \
	src/movement/movement_tracks.c \
	src/movement/movement_events.c \
	src/combat/act.offensive.c \
	src/act.other.c \
	src/act.social.c \
	src/act.wizard.c \
	src/actionqueues.c \
	src/actions.c \
	src/olc/aedit.c \
	src/ai_cache.c \
	src/ai_events.c \
	src/ai_security.c \
	src/ai_service.c \
	src/craft/alchemy.c \
	src/asciimap.c \
	src/combat/assign_wpn_armor.c \
	src/character/abilities.c \
	src/character/backgrounds.c \
	src/character/guild_services.c \
	src/character/character_creation.c \
	src/character/character_creation_content.c \
	src/ban.c \
	src/bardic_performance.c \
	src/comms/boards.c \
	src/olc/bedit.c \
	src/comms/mysql_boards.c \
	src/bsd-snprintf.c \
	src/craft/brew.c \
	src/olc/cedit.c \
	src/char_descs.c \
	src/clan.c \
	src/clan_services.c \
	src/clan_economy.c \
	src/olc/clan_edit.c \
	src/clan_transactions.c \
	src/character/class.c \
	src/combat/combat_modes.c \
	src/comm.c \
	src/config.c \
	src/constants.c \
	src/copyover_diagnostic.c \
	src/craft/craft.c \
	src/craft/crafting_molds.c \
	src/craft/crafting_new.c \
	src/craft/crafting_recipes.c \
	src/craft/crafts.c \
	src/db.c \
	src/db_init.c \
	src/db_init_data.c \
	src/db_startup_init.c \
	src/db_admin_commands.c \
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
	src/dotenv.c \
	src/combat/encounters.c \
	src/character/evolutions.c \
	src/character/feats.c \
	src/character/talents.c \
	src/character/perks.c \
	src/combat/fight.c \
	src/olc/genmob.c \
	src/olc/genobj.c \
	src/olc/genolc.c \
	src/olc/genqst.c \
	src/olc/genshp.c \
	src/olc/genwld.c \
	src/olc/genzon.c \
	src/graph.c \
	src/combat/grapple.c \
	src/handler.c \
	src/olc/hedit.c \
	src/help.c \
	src/helpers.c \
	src/olc/hlqedit.c \
	src/quest/hlquest.c \
	src/obj/house.c \
	src/olc/hsedit.c \
	src/quest/hunts.c \
	src/comms/ibt.c \
	src/olc/improved-edit.c \
	src/interpreter.c \
	src/introduce.c \
	src/wilderness/kdtree.c \
	src/limits.c \
	src/lists.c \
	src/magic/magic.c \
	src/comms/mail.c \
	src/olc/medit.c \
	src/magic/metamagic_science.c \
	src/quest/missions.c \
	src/mob/mob_act.c \
	src/mob/mob_memory.c \
	src/mob/mob_utils.c \
	src/mob/mob_race.c \
	src/mob/mob_psionic.c \
	src/mob/mob_class.c \
	src/mob/mob_spells.c \
	src/mob/mob_spellslots.c \
	src/mob/mob_known_spells.c \
	src/modify.c \
	src/magic/moon_bonus_spells.c \
	src/olc/msgedit.c \
	src/mud_event.c \
	src/mud_event_list.c \
	src/mysql.c \
	src/comms/new_mail.c \
	src/olc/oasis.c \
	src/olc/oasis_copy.c \
	src/olc/oasis_delete.c \
	src/olc/oasis_list.c \
	src/obj/objsave.c \
	src/obj/player_shop.c \
	src/olc/oedit.c \
	src/perfmon.c \
	src/wilderness/perlin.c \
	src/player_rename.c \
	src/player_rename.h \
	src/players.c \
	src/olc/prefedit.c \
	src/character/premadebuilds.c \
	src/net/msdp_json.c \
	src/net/msdp_json.h \
	src/net/protocol.c \
	src/magic/psionics.c \
	src/pubsub/pubsub.c \
	src/pubsub/pubsub_handlers.c \
	src/pubsub/pubsub_db.c \
	src/pubsub/pubsub_commands.c \
	src/pubsub/pubsub_queue.c \
	src/pubsub/pubsub_spatial.c \
	src/pubsub/pubsub_event_handlers.c \
	src/wilderness/spatial_core.c \
	src/wilderness/spatial_visual.c \
	src/wilderness/spatial_audio.c \
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
	src/random.c \
	src/random_names.c \
	src/rank.c \
	src/olc/redit.c \
	src/wilderness/resource_system.c \
	src/wilderness/resource_depletion.c \
	src/wilderness/resource_descriptions.c \
	src/wilderness/resource_regeneration.c \
	src/roleplay.c \
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
	src/spec/spec_rol_combat.c \
	src/spec/spec_rol_conversion.c \
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
	src/utils.c \
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
	src/weather.c \
	src/wilderness/wilderness.c \
	src/wilderness/wilderness_kb.c \
	src/olc/zedit.c \
	src/zmalloc.c

# Libraries to link
circle_LDADD = @LIBS@ @CRYPTLIB@ @NETLIB@ -lcrypt -lgd -lm -lmariadb -lcurl -lssl -lcrypto -lpthread -ljson-c


# Unit testing with CuTest
check_PROGRAMS = cutest

# CuTest sources
cutest_SOURCES = \
	unittests/CuTest/test_syntax_check_boot.c \
	unittests/CuTest/test_web_onboarding.c \
	unittests/CuTest/test_i3_client_production.c \
	unittests/CuTest/test_msdp_production.c \
	unittests/CuTest/test_necromancer.c \
	unittests/CuTest/CuTest.c \
	unittests/CuTest/test_bounds_checking.c \
	unittests/CuTest/test_artifacts.c \
	unittests/CuTest/test_artifact_integration.c \
	unittests/CuTest/test_player_rename.c \
	unittests/CuTest/test.helpers.c \
	unittests/CuTest/test.interpreter.c \
	unittests/CuTest/test_transport_production.c \
	unittests/CuTest/test_traps_production.c \
	unittests/CuTest/test_combat_production.c \
	unittests/CuTest/test_copyover_timer.c \
	unittests/CuTest/test_bardic_performance.c \
	unittests/CuTest/test_spells_skills_production.c \
	unittests/CuTest/test_perfmon_production.c \
	unittests/CuTest/test_vessel_events.c \
	unittests/CuTest/test_vessel_tactical.c \
	unittests/CuTest/test_vessel_lookout.c \
	unittests/CuTest/test_vessel_narrative.c \
	unittests/CuTest/test_vessel_boarding.c \
	unittests/CuTest/test_database_persistence.c \
	unittests/CuTest/test_dg_scripts_production.c \
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
	unittests/CuTest/AllTests.c \
	$(circle_SOURCES)

cutest_CFLAGS = $(AM_CFLAGS) -DLUMINARI_CUTEST
cutest_LDADD = $(circle_LDADD)

cutest_test_files = \
	unittests/CuTest/test_syntax_check_boot.c \
	unittests/CuTest/test_bounds_checking.c \
	unittests/CuTest/test_web_onboarding.c \
	unittests/CuTest/test_i3_client_production.c \
	unittests/CuTest/test_msdp_production.c \
	unittests/CuTest/test_necromancer.c \
	unittests/CuTest/test_artifacts.c \
	unittests/CuTest/test_artifact_integration.c \
	unittests/CuTest/test_player_rename.c \
	unittests/CuTest/test.helpers.c \
	unittests/CuTest/test.interpreter.c \
	unittests/CuTest/test_transport_production.c \
	unittests/CuTest/test_traps_production.c \
	unittests/CuTest/test_combat_production.c \
	unittests/CuTest/test_copyover_timer.c \
	unittests/CuTest/test_bardic_performance.c \
	unittests/CuTest/test_spells_skills_production.c \
	unittests/CuTest/test_perfmon_production.c \
	unittests/CuTest/test_vessel_events.c \
	unittests/CuTest/test_vessel_tactical.c \
	unittests/CuTest/test_vessel_lookout.c \
	unittests/CuTest/test_vessel_narrative.c \
	unittests/CuTest/test_vessel_boarding.c \
	unittests/CuTest/test_database_persistence.c \
	unittests/CuTest/test_dg_scripts_production.c \
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
	unittests/CuTest/test_gameplay_e2e.c

# Generate AllTests.c before building cutest
unittests/CuTest/AllTests.c: $(cutest_test_files) unittests/CuTest/make-tests.sh
	unittests/CuTest/make-tests.sh $(cutest_test_files) > $@

BUILT_SOURCES = build_identity.h unittests/CuTest/AllTests.c

# Clean up generated files
CLEANFILES = build_identity.h unittests/CuTest/AllTests.c

.PHONY: FORCE_BUILD_IDENTITY
FORCE_BUILD_IDENTITY:

build_identity.h: FORCE_BUILD_IDENTITY scripts/deployment/generate_build_identity.sh
	$(AM_V_GEN)$(SHELL) $(abs_top_srcdir)/scripts/deployment/generate_build_identity.sh \
		$@ $(abs_top_srcdir)

# Direct parallel targets such as `make -j cutest` do not receive Automake's
# BUILT_SOURCES ordering.  Both variants of constants.o include this header.
src/constants.$(OBJEXT) src/cutest-constants.$(OBJEXT): build_identity.h

# Stage the server away from the mutable launch alias. The install hook below
# promotes it into an immutable build-ID release and updates bin/circle.
bindir = $(top_builddir)/bin/.circle-install

# After installation, remove the executable from the source directory
install-exec-hook:
	$(SHELL) $(abs_top_srcdir)/scripts/deployment/install_versioned_binary.sh \
		$(bindir)/circle $(top_builddir)/bin
	$(SHELL) $(abs_top_srcdir)/scripts/provision_artifacts.sh
	rm -f $(bindir)/circle
	rmdir $(bindir)
	rm -f $(bin_PROGRAMS)

# Ensure constants.c is rebuilt when other sources change
src/constants.c: $(filter-out src/constants.c,$(circle_SOURCES))
	touch $@

# Standalone Python world-data tooling. Keep this list synchronized with
# WORLD_TOOL_SOURCES in CMakeLists.txt.
world_tool_sources = \
	scripts/world/generate_rol_periodic_profiles.py \
	scripts/world/generate_rol_state_periodic_profiles.py \
	scripts/world/wtool.py \
	scripts/world/wtool_constants.json \
	scripts/world/rol_conversion_policy.json \
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
	scripts/world/wtool_lib/rol_inventory.py \
	scripts/world/wtool_lib/rol_planner.py \
	scripts/world/wtool_lib/rol_pilot.py \
	scripts/world/wtool_lib/rol_pilot_build.py \
	scripts/world/wtool_lib/rol_periodic_profiles.py \
	scripts/world/wtool_lib/rol_skeleton.py \
	scripts/world/wtool_lib/rol_state_periodic_profiles.py \
	scripts/world/wtool_lib/rol_soc.py \
	scripts/world/wtool_lib/rol_special.py \
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
	scripts/world/tests/test_rol_capability_audit.py \
	scripts/world/tests/test_rol_discovery.py \
	scripts/world/tests/test_rol_inventory.py \
	scripts/world/tests/test_rol_planner.py \
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

# Extra files to distribute
EXTRA_DIST = \
	$(world_tool_sources) \
	$(world_tool_support_files) \
	lib/world/artifacts/1699.mob \
	lib/world/artifacts/1699.obj \
	lib/world/artifacts/1699.wld \
	lib/world/artifacts/1699.zon \
	lib/world/artifacts/artifacts.hlp \
	unittests/README_unittests.md \
	unittests/CuTest/Makefile \
	unittests/CuTest/README.txt \
	unittests/CuTest/fuzz_protocol_parser.c \
	unittests/CuTest/fuzz_corpus/plain_command.txt \
	unittests/CuTest/fuzz_corpus/gmcp_like.txt \
	unittests/CuTest/fuzz_corpus/output_unicode.txt \
	unittests/CuTest/fixtures/spec_world_inventory/mob/inventory.mob \
	unittests/CuTest/fixtures/spec_world_inventory/obj/inventory.obj \
	unittests/CuTest/fixtures/spec_world_inventory/wld/inventory.wld \
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
	scripts/ci/prepare_test_runtime.sh \
	scripts/debugging/debug_game.sh \
	scripts/debugging/verify_core_capture.sh \
	scripts/debugging/vgrind.sh \
	scripts/deploy.sh \
	scripts/deployment/deploy.sh \
	scripts/deployment/generate_build_identity.sh \
	scripts/deployment/install_versioned_binary.sh \
	scripts/deployment/move_bin.sh \
	scripts/deployment/setup.sh \
	scripts/deployment/test_versioned_binary_install.sh \
	scripts/development/dev_create_test_character.sh \
	scripts/development/dev_kohdee_login_smoke.sh \
	scripts/mariadb/setup_mariadb_sudoers.sh \
	scripts/mariadb/start_mariadb.sh \
	scripts/move_bin.sh \
	scripts/operations/healthcheck.sh \
	scripts/operations/test_healthcheck.sh \
	scripts/permissions/check_permissions.sh \
	scripts/permissions/fix_permissions.sh \
	scripts/process-memory/sample_process_memory_details.sh \
	scripts/process-memory/test_process_memory_details.sh \
	scripts/provision_artifacts.sh \
	scripts/setup.sh \
	scripts/test_background_help_entries.sh \
	scripts/test_pubsub.sh \
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
	sql/components/help_specproc_entries.sql \
	sql/components/help_vessel_entries.sql \
	sql/components/help_character_backgrounds.sql \
	sql/components/help_necromancer_entries.sql \
	sql/components/help_rol_object_trap_entries.sql \
	sql/components/ci_schema_manifest.txt \
	sql/components/test_vessels_integrity.sql \
	sql/components/verify_help_necromancer_entries.sql \
	sql/components/verify_help_rol_object_trap_entries.sql \
	sql/components/verify_help_specproc_entries.sql \
	sql/components/verify_help_vessel_entries.sql \
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

# Run unit tests
test: check test-autorun-supervision test-versioned-binary-install test-healthcheck \
	test-background-help-entries test-vessel-tooling
	LUMINARI_TEST_ROOT="$(abs_top_srcdir)" ./cutest

.PHONY: check-world-docs test-all test-world-tools test-protocol test-protocol-fuzz test-character-rename-static \
	test-character-rename-schema test-background-help-entries test-autorun-supervision \
	test-versioned-binary-install test-healthcheck test-vessel-tooling

check-world-docs:
	cd "$(abs_top_srcdir)" && python3 scripts/world/wtool.py constants sync --check
	cd "$(abs_top_srcdir)" && python3 scripts/world/wtool.py docs --check

test-world-tools: check-world-docs
	cd "$(abs_top_srcdir)" && python3 -m unittest discover -s scripts/world/tests -t scripts/world -v
	"$(abs_top_srcdir)/lib/world/validate-zone.sh" 100 \
		--world-root "$(abs_top_srcdir)/scripts/world/tests/fixtures/phase2/complete" \
		--json >/dev/null

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

test-autorun-supervision:
	./scripts/autorun/test_autorun_supervision.sh

test-versioned-binary-install:
	./scripts/deployment/test_versioned_binary_install.sh

test-healthcheck:
	./scripts/operations/test_healthcheck.sh

test-vessel-tooling:
	./scripts/vessels/test_vessel_ferry_soak_preflight.sh
	./scripts/vessels/test_vessel_memory_analyzer.sh
	./scripts/process-memory/test_process_memory_details.sh
	./scripts/vessels/test_vessel_scale_benchmark_parsers.sh

# Authoritative full local test entry point. The final install both verifies the
# install path and removes the root-level circle artifact created by make test.
test-all:
	$(MAKE) test
	$(MAKE) test-world-tools
	$(MAKE) test-protocol
	$(MAKE) test-character-rename-static
	$(MAKE) test-character-rename-schema
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
	rm -f src/*~ src/*.swp src/*.swo
	rm -f *.tmp *.temp
	rm -f test_*.out test_*.log
