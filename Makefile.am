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
	src/character/backgrounds.c \
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
	src/olc/oedit.c \
	src/perfmon.c \
	src/wilderness/perlin.c \
	src/player_rename.c \
	src/player_rename.h \
	src/players.c \
	src/olc/prefedit.c \
	src/character/premadebuilds.c \
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
	src/obj/shop.c \
	src/combat/spec_abilities.c \
	src/spec_assign.c \
	src/spec_procs.c \
	src/obj/spec_artifacts.c \
	src/magic/casting_visuals.c \
	src/magic/spell_parser.c \
	src/magic/spell_prep.c \
	src/magic/spellbook_scroll.c \
	src/magic/spells.c \
	src/quest/staff_events.c \
	src/character/study.c \
	src/olc/tedit.c \
	src/character/templates.c \
	src/obj/trade.c \
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
	src/zmalloc.c \
	src/zone_procs.c

# Libraries to link
circle_LDADD = @LIBS@ @CRYPTLIB@ @NETLIB@ -lcrypt -lgd -lm -lmariadb -lcurl -lssl -lcrypto -lpthread -ljson-c


# Unit testing with CuTest
check_PROGRAMS = cutest

# CuTest sources
cutest_SOURCES = \
	unittests/CuTest/test_web_onboarding.c \
	unittests/CuTest/test_i3_client_production.c \
	unittests/CuTest/CuTest.c \
	unittests/CuTest/test_bounds_checking.c \
	unittests/CuTest/test_artifacts.c \
	unittests/CuTest/test_player_rename.c \
	unittests/CuTest/test.helpers.c \
	unittests/CuTest/test.interpreter.c \
	unittests/CuTest/test_transport_production.c \
	unittests/CuTest/test_traps_production.c \
	unittests/CuTest/test_combat_production.c \
	unittests/CuTest/test_copyover_timer.c \
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
	unittests/CuTest/test_gameplay_e2e.c \
	unittests/CuTest/AllTests.c \
	$(circle_SOURCES)

cutest_CFLAGS = $(AM_CFLAGS) -DLUMINARI_CUTEST
cutest_LDADD = $(circle_LDADD)

cutest_test_files = \
	unittests/CuTest/test_bounds_checking.c \
	unittests/CuTest/test_web_onboarding.c \
	unittests/CuTest/test_i3_client_production.c \
	unittests/CuTest/test_artifacts.c \
	unittests/CuTest/test_player_rename.c \
	unittests/CuTest/test.helpers.c \
	unittests/CuTest/test.interpreter.c \
	unittests/CuTest/test_transport_production.c \
	unittests/CuTest/test_traps_production.c \
	unittests/CuTest/test_combat_production.c \
	unittests/CuTest/test_copyover_timer.c \
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
	unittests/CuTest/test_gameplay_e2e.c

# Generate AllTests.c before building cutest
unittests/CuTest/AllTests.c: $(cutest_test_files) unittests/CuTest/make-tests.sh
	unittests/CuTest/make-tests.sh $(cutest_test_files) > $@

BUILT_SOURCES = unittests/CuTest/AllTests.c

# Clean up generated files
CLEANFILES = unittests/CuTest/AllTests.c

# Install to local bin directory
bindir = $(top_builddir)/bin

# After installation, remove the executable from the source directory
install-exec-hook:
	$(SHELL) $(abs_top_srcdir)/scripts/provision_artifacts.sh
	rm -f $(bin_PROGRAMS)

# Ensure constants.c is rebuilt when other sources change
src/constants.c: $(filter-out src/constants.c,$(circle_SOURCES))
	touch $@

# Extra files to distribute
EXTRA_DIST = \
	unittests/README.md \
	unittests/CuTest/Makefile \
	unittests/CuTest/README.txt \
	unittests/CuTest/fuzz_protocol_parser.c \
	unittests/CuTest/fuzz_corpus/plain_command.txt \
	unittests/CuTest/fuzz_corpus/gmcp_like.txt \
	unittests/CuTest/make-tests.sh \
	unittests/CuTest/test_protocol_parser.c \
	scripts/analyze_vessel_memory_samples.sh \
	scripts/dev_create_test_character.sh \
	scripts/dev_kohdee_login_smoke.sh \
	scripts/test_character_rename_schema.sh \
	scripts/test_character_rename_static.sh \
	scripts/test_background_help_entries.sh \
	scripts/test_autorun_supervision.sh \
	scripts/provision_artifacts.sh \
	scripts/provision_vessel_harbor.sh \
	scripts/run_vessel_ferry_soak.sh \
	scripts/run_vessel_scale_benchmark.sh \
	scripts/sample_process_memory_details.sh \
	scripts/test_process_memory_details.sh \
	scripts/test_vessel_ferry_soak_preflight.sh \
	scripts/test_vessel_hunter_in_game.sh \
	scripts/test_vessel_merchant_in_game.sh \
	scripts/test_vessel_events_in_game.sh \
	scripts/test_vessel_tactical_in_game.sh \
	scripts/test_vessel_lookout_in_game.sh \
	scripts/test_vessel_narrative_in_game.sh \
	scripts/test_vessel_memory_analyzer.sh \
	scripts/test_vessel_scale_benchmark_parsers.sh \
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
	sql/components/help_vessel_entries.sql \
	sql/components/help_character_backgrounds.sql \
	sql/components/ci_schema_manifest.txt \
	sql/components/test_vessels_integrity.sql \
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
test: check test-autorun-supervision test-background-help-entries test-vessel-tooling
	LUMINARI_TEST_ROOT="$(abs_top_srcdir)" ./cutest

.PHONY: test-all test-protocol test-protocol-fuzz test-character-rename-static \
	test-character-rename-schema test-background-help-entries test-autorun-supervision \
	test-vessel-tooling

test-protocol:
	$(MAKE) -C "$(abs_top_srcdir)/unittests/CuTest" protocol-parser

test-protocol-fuzz:
	$(MAKE) -C "$(abs_top_srcdir)/unittests/CuTest" protocol-fuzz

test-character-rename-static:
	./scripts/test_character_rename_static.sh

test-character-rename-schema:
	./scripts/test_character_rename_schema.sh

test-background-help-entries:
	./scripts/test_background_help_entries.sh

test-autorun-supervision:
	./scripts/test_autorun_supervision.sh

test-vessel-tooling:
	./scripts/test_vessel_ferry_soak_preflight.sh
	./scripts/test_vessel_memory_analyzer.sh
	./scripts/test_process_memory_details.sh
	./scripts/test_vessel_scale_benchmark_parsers.sh

# Authoritative full local test entry point. The final install both verifies the
# install path and removes the root-level circle artifact created by make test.
test-all:
	$(MAKE) test
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
