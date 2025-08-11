# LuminariMUD Makefile.am - for use with automake
# Based on the original Makefile.in structure

# Set up subdirectories
SUBDIRS = util

# Additional compiler flags
# Revert to project defaults: do NOT force global feature defines here.
AM_CFLAGS = -std=gnu90 -Isrc @MYFLAGS@

# Build timestamp information
MKTIME = $(shell date +%s)
MKUSER = $(USER)
MKHOST = $(HOSTNAME)
BRANCH = $(shell git branch 2>/dev/null | grep '*' | cut -d' ' -f2 || echo "unknown")
PARENT = $(shell git rev-parse HEAD 2>/dev/null || echo "unknown")

# Main program
bin_PROGRAMS = circle

# Source files for the main program
circle_SOURCES = \
	src/account.c \
	src/act.comm.c \
	src/act.comm.do_spec_comm.c \
	src/act.informative.c \
	src/act.item.c \
	src/act.movement.c \
	src/act.offensive.c \
	src/act.other.c \
	src/act.social.c \
	src/act.wizard.c \
	src/actionqueues.c \
	src/actions.c \
	src/aedit.c \
	src/ai_cache.c \
	src/ai_events.c \
	src/ai_security.c \
	src/ai_service.c \
	src/alchemy.c \
	src/asciimap.c \
	src/assign_wpn_armor.c \
	src/backgrounds.c \
	src/ban.c \
	src/bardic_performance.c \
	src/boards.c \
	src/bsd-snprintf.c \
	src/cedit.c \
	src/char_descs.c \
	src/clan.c \
	src/clan_economy.c \
	src/clan_edit.c \
	src/clan_transactions.c \
	src/class.c \
	src/combat_modes.c \
	src/comm.c \
	src/config.c \
	src/constants.c \
	src/craft.c \
	src/crafting_new.c \
	src/crafting_recipes.c \
	src/crafts.c \
	src/db.c \
	src/deities.c \
	src/desc_engine.c \
	src/dg_comm.c \
	src/dg_db_scripts.c \
	src/dg_event.c \
	src/dg_handler.c \
	src/dg_misc.c \
	src/dg_mobcmd.c \
	src/dg_objcmd.c \
	src/dg_olc.c \
	src/dg_scripts.c \
	src/dg_triggers.c \
	src/dg_variables.c \
	src/dg_wldcmd.c \
	src/domain_powers.c \
	src/domains_schools.c \
	src/dotenv.c \
	src/encounters.c \
	src/evolutions.c \
	src/feats.c \
	src/fight.c \
	src/gain.c \
	src/genmob.c \
	src/genobj.c \
	src/genolc.c \
	src/genqst.c \
	src/genshp.c \
	src/genwld.c \
	src/genzon.c \
	src/graph.c \
	src/grapple.c \
	src/handler.c \
	src/hedit.c \
	src/help.c \
	src/helpers.c \
	src/hlqedit.c \
	src/hlquest.c \
	src/house.c \
	src/hsedit.c \
	src/hunts.c \
	src/ibt.c \
	src/improved-edit.c \
	src/interpreter.c \
	src/kdtree.c \
	src/limits.c \
	src/lists.c \
	src/magic.c \
	src/mail.c \
	src/medit.c \
	src/missions.c \
	src/mobact.c \
	src/modify.c \
	src/msgedit.c \
	src/mud_event.c \
	src/mud_event_list.c \
	src/mysql.c \
	src/new_mail.c \
	src/oasis.c \
	src/oasis_copy.c \
	src/oasis_delete.c \
	src/oasis_list.c \
	src/objsave.c \
	src/oedit.c \
	src/perfmon.c \
	src/perlin.c \
	src/players.c \
	src/prefedit.c \
	src/premadebuilds.c \
	src/protocol.c \
	src/psionics.c \
	src/qedit.c \
	src/quest.c \
	src/race.c \
	src/random.c \
	src/random_names.c \
	src/rank.c \
	src/redit.c \
	src/resource_system.c \
	src/resource_depletion.c \
	src/resource_descriptions.c \
	src/roleplay.c \
	src/routing.c \
	src/sedit.c \
	src/shop.c \
	src/spec_abilities.c \
	src/spec_assign.c \
	src/spec_procs.c \
	src/specs.artifacts.c \
	src/spell_parser.c \
	src/spell_prep.c \
	src/spellbook_scroll.c \
	src/spells.c \
	src/staff_events.c \
	src/study.c \
	src/tedit.c \
	src/templates.c \
	src/trade.c \
	src/transport.c \
	src/traps.c \
	src/treasure.c \
	src/treasure_const.c \
	src/utils.c \
	src/weather.c \
	src/wilderness.c \
	src/zedit.c \
	src/zmalloc.c \
	src/zone_procs.c

# Libraries to link
circle_LDADD = @LIBS@ @CRYPTLIB@ @NETLIB@ -lcrypt -lgd -lm -lmysqlclient -lcurl -lssl -lcrypto -lpthread

# Special flags for constants.c to include build information
src/constants.$(OBJEXT): AM_CFLAGS += -DMKTIME='"$(MKTIME)"' -DMKUSER='"$(MKUSER)"' -DMKHOST='"$(MKHOST)"' -DBRANCH='"$(BRANCH)"' -DPARENT='"$(PARENT)"'

# Unit testing with CuTest
check_PROGRAMS = cutest

# CuTest sources
cutest_SOURCES = \
	unittests/CuTest/CuTest.c \
	unittests/CuTest/test_alignment.c \
	unittests/CuTest/test_bounds_checking.c \
	unittests/CuTest/test_char_cache.c \
	unittests/CuTest/test_char_skills.c \
	unittests/CuTest/test_random.c \
	unittests/CuTest/test_treasure.c \
	unittests/CuTest/test_utils.c \
	unittests/CuTest/test.helpers.c \
	unittests/CuTest/test.interpreter.c \
	unittests/CuTest/AllTests.c \
	$(circle_SOURCES)

cutest_CFLAGS = $(AM_CFLAGS) -DLUMINARI_CUTEST
cutest_LDADD = $(circle_LDADD)

# Generate AllTests.c before building cutest
unittests/CuTest/AllTests.c: $(filter-out unittests/CuTest/AllTests.c,$(wildcard unittests/CuTest/*.c))
	unittests/CuTest/make-tests.sh unittests/CuTest/*.c > $@

BUILT_SOURCES = unittests/CuTest/AllTests.c

# Clean up generated files
CLEANFILES = unittests/CuTest/AllTests.c

# Install to local bin directory
bindir = $(top_builddir)/bin

# After installation, remove the executable from the source directory
install-exec-hook:
	rm -f $(bin_PROGRAMS)

# Ensure constants.c is rebuilt when other sources change
src/constants.c: $(filter-out src/constants.c,$(circle_SOURCES))
	touch $@

# Extra files to distribute
EXTRA_DIST = \
	unittests/CuTest/make-tests.sh \
	README.md \
	CHANGELOG.md \
	LICENSE

# Run unit tests
test: check
	./cutest

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
scrub: clean
	rm -f gdb.txt valgrind.log WORK.md CODEBASE_SCOPE.md
	rm -f configure~ configure.ac~ conf.h.in~
	rm -f config.log
	rm -rf autom4te.cache/
	rm -f log/*.log
	rm -f *.orig *.rej *.patch
	rm -f .killscript .fastboot pause .autorun.lock
	rm -f .websocket_policy.pid .flash_policy.pid
	rm -f lib/copyover.dat
	rm -f core core.* vgcore.*
	rm -f lib/core lib/core.* lib/vgcore.*
	rm -f *~ *.swp *.swo
	rm -f src/*~ src/*.swp src/*.swo
	rm -f *.tmp *.temp
	rm -f test_*.out test_*.log
