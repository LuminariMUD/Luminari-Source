cmake_minimum_required(VERSION 3.21)
# Version: Keep in sync with src/constants.c, src/structs.h, configure.ac
project(LuminariMUD VERSION 2.5063 LANGUAGES C)

# Build as GNU C23 while retaining the project's established source style.
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# ========== User-facing options ==========
# Every option is documented in docs/development/CMAKE_BUILD_GUIDE.md.
option(BUILD_UTILS "Build the util/ helper programs" ON)
option(BUILD_TESTS "Build production-linked unit tests and register CTest entries" OFF)
option(DEVELOPER_MODE "Enable additional diagnostic warnings for development builds" OFF)
option(MEMORY_DEBUG "Compile with the in-tree MEMORY_DEBUG allocation tracing" OFF)
option(DMALLOC "Link against the dmalloc debugging allocator" OFF)
option(STATIC_ANALYSIS "Run clang-tidy on every compiled source" OFF)
option(LUMINARI_WERROR "Treat compiler warnings as errors" OFF)
option(LUMINARI_COVERAGE "Instrument all targets for gcov/gcovr line and branch coverage" OFF)
option(LUMINARI_HARDENING "Apply hardened Release flags (fortify, stack protector, PIE, RELRO)" OFF)
set(LUMINARI_SANITIZERS "" CACHE STRING
    "Comma-separated -fsanitize= list applied to every target (for example address,undefined)")

include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckTypeSize)
include(CheckStructHasMember)
include(CheckSymbolExists)
include(CheckCSourceCompiles)
include(CheckCCompilerFlag)

check_c_source_compiles(
    "int main(void) {
       bool value = true;
       void *ptr = nullptr;
       static_assert(sizeof(bool) > 0);
       return value && ptr == nullptr ? 0 : 1;
     }"
    LUMINARI_HAVE_C23_KEYWORDS)
if (NOT LUMINARI_HAVE_C23_KEYWORDS)
    message(FATAL_ERROR "The C compiler does not provide the required GNU C23 language support")
endif()

# Build outside the mutable launch alias. Installation promotes luminari into an
# immutable build-ID release under the source tree's bin directory.
set(BIN_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)
set(INSTALL_BIN_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})

set(BUILD_IDENTITY_HEADER ${CMAKE_CURRENT_BINARY_DIR}/build_identity.h)
execute_process(
    COMMAND bash
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/deployment/generate_build_identity.sh
            ${BUILD_IDENTITY_HEADER}
            ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE BUILD_IDENTITY_RESULT)
if (NOT BUILD_IDENTITY_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate build identity")
endif()

execute_process(
    COMMAND git -C ${CMAKE_CURRENT_SOURCE_DIR} rev-parse --git-path HEAD
    OUTPUT_VARIABLE GIT_HEAD_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
execute_process(
    COMMAND git -C ${CMAKE_CURRENT_SOURCE_DIR} symbolic-ref -q HEAD
    OUTPUT_VARIABLE GIT_HEAD_REFERENCE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
if (GIT_HEAD_PATH)
    if (NOT IS_ABSOLUTE ${GIT_HEAD_PATH})
        set(GIT_HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${GIT_HEAD_PATH})
    endif()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${GIT_HEAD_PATH})
endif()
if (GIT_HEAD_REFERENCE)
    execute_process(
        COMMAND git -C ${CMAKE_CURRENT_SOURCE_DIR} rev-parse --git-path ${GIT_HEAD_REFERENCE}
        OUTPUT_VARIABLE GIT_HEAD_REFERENCE_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if (GIT_HEAD_REFERENCE_PATH)
        if (NOT IS_ABSOLUTE ${GIT_HEAD_REFERENCE_PATH})
            set(GIT_HEAD_REFERENCE_PATH
                ${CMAKE_CURRENT_SOURCE_DIR}/${GIT_HEAD_REFERENCE_PATH})
        endif()
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                     ${GIT_HEAD_REFERENCE_PATH})
    endif()
endif()

# Check for required configuration headers
if (NOT EXISTS ${CMAKE_SOURCE_DIR}/src/campaign.h)
    message(FATAL_ERROR "campaign.h not found! Copy src/campaign.example.h to src/campaign.h and configure it.")
endif()
if (NOT EXISTS ${CMAKE_SOURCE_DIR}/src/mud_options.h)
    message(FATAL_ERROR "mud_options.h not found! Copy src/mud_options.example.h to src/mud_options.h and configure it.")
endif()
if (NOT EXISTS ${CMAKE_SOURCE_DIR}/src/vnums.h)
    message(FATAL_ERROR "vnums.h not found! Copy src/vnums.example.h to src/vnums.h and configure it.")
endif()

# ========== Dependencies ==========
# Autotools links every one of these unconditionally (luminari_LDADD in
# Makefile.am), so they are required here as well. Minimum versions are the
# oldest releases the supported Ubuntu LTS images ship; see
# docs/development/CMAKE_BUILD_GUIDE.md.
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(CURL 7.68 REQUIRED)
find_package(OpenSSL 1.1.1 REQUIRED)
pkg_check_modules(LIBEVENT REQUIRED IMPORTED_TARGET libevent_core>=2.1.12)
pkg_check_modules(JSONC REQUIRED IMPORTED_TARGET json-c>=0.13)
pkg_check_modules(GD REQUIRED IMPORTED_TARGET gdlib>=2.2)
pkg_check_modules(MARIADB IMPORTED_TARGET libmariadb>=3.1)
if (MARIADB_FOUND)
    set(LUMINARI_MYSQL_TARGET PkgConfig::MARIADB)
else()
    pkg_check_modules(MYSQLCLIENT REQUIRED IMPORTED_TARGET mysqlclient>=3.1)
    set(LUMINARI_MYSQL_TARGET PkgConfig::MYSQLCLIENT)
endif()
find_library(CRYPT_LIBRARY crypt REQUIRED)

add_library(luminari_deps INTERFACE)
target_link_libraries(luminari_deps INTERFACE
    ${LUMINARI_MYSQL_TARGET}
    PkgConfig::GD
    PkgConfig::JSONC
    PkgConfig::LIBEVENT
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    ${CRYPT_LIBRARY}
    m)
if (DMALLOC)
    find_library(DMALLOC_LIBRARY dmalloc REQUIRED)
    target_link_libraries(luminari_deps INTERFACE ${DMALLOC_LIBRARY})
endif()

# ========== Platform feature checks (mirror configure.ac) ==========
foreach(HEADER
        fcntl.h sys/fcntl.h errno.h net/errno.h string.h strings.h
        limits.h sys/time.h sys/select.h sys/types.h unistd.h
        memory.h crypt.h assert.h arpa/telnet.h arpa/inet.h
        sys/stat.h sys/socket.h sys/resource.h netinet/in.h netdb.h
        signal.h sys/uio.h mcheck.h sys/wait.h time.h
        stdio.h stdlib.h inttypes.h stdint.h)
    string(TOUPPER "${HEADER}" _upper_name)
    string(REGEX REPLACE "[./]" "_" _upper_name "${_upper_name}")
    check_include_file("${HEADER}" HAVE_${_upper_name})
endforeach()

if (HAVE_STDLIB_H AND HAVE_STDIO_H AND HAVE_STRING_H)
    set(STDC_HEADERS 1)
endif()

set(CMAKE_REQUIRED_LIBRARIES ${CRYPT_LIBRARY})
check_function_exists(crypt CIRCLE_CRYPT)
unset(CMAKE_REQUIRED_LIBRARIES)

set(CMAKE_REQUIRED_LIBRARIES PkgConfig::JSONC)
check_function_exists(json_object_new_object HAVE_JSON_C)
unset(CMAKE_REQUIRED_LIBRARIES)

foreach(FUNC gettimeofday select snprintf strcasecmp strdup strerror
        stricmp strlcpy strncasecmp strnicmp strstr vsnprintf vprintf
        inet_addr inet_aton gethostbyaddr socket)
    string(TOUPPER "${FUNC}" _upper_name)
    check_function_exists(${FUNC} HAVE_${_upper_name})
endforeach()
if (NOT HAVE_GETHOSTBYADDR)
    find_library(NSL_LIBRARY nsl REQUIRED)
    target_link_libraries(luminari_deps INTERFACE ${NSL_LIBRARY})
endif()
if (NOT HAVE_SOCKET)
    find_library(SOCKET_LIBRARY socket REQUIRED)
    target_link_libraries(luminari_deps INTERFACE ${SOCKET_LIBRARY})
endif()

# Missing system types become the same conf.h fallbacks configure.ac emits.
check_type_size("pid_t" HAVE_PID_T)
check_type_size("size_t" HAVE_SIZE_T)
check_type_size("ssize_t" HAVE_SSIZE_T)
set(CMAKE_EXTRA_INCLUDE_FILES "sys/socket.h")
check_type_size("socklen_t" HAVE_SOCKLEN_T)
unset(CMAKE_EXTRA_INCLUDE_FILES)
if (NOT HAVE_PID_T)
    set(pid_t int)
endif()
if (NOT HAVE_SIZE_T)
    set(size_t "unsigned int")
endif()
if (NOT HAVE_SSIZE_T)
    set(ssize_t int)
endif()
if (NOT HAVE_SOCKLEN_T)
    set(socklen_t int)
endif()
if (HAVE_NETINET_IN_H)
    check_struct_has_member("struct in_addr" s_addr netinet/in.h HAVE_STRUCT_IN_ADDR)
endif()
if (UNIX)
    set(CIRCLE_UNIX 1)
endif()

# Generate the header included by both the server and utility sources after all
# platform checks have populated their feature variables. It lands at
# src/conf.h, the same gitignored location configure.ac uses, because the
# standalone test scripts under scripts/ preprocess sources with -Isrc.
configure_file(cmake/cmake_config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/conf.h)

# ========== Shared build interface ==========
# Every compiled target links this interface library instead of relying on
# directory-wide include paths or global CMAKE_C_FLAGS edits.
add_library(luminari_build INTERFACE)
target_include_directories(luminari_build INTERFACE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(LUMINARI_GNU_LIKE "$<C_COMPILER_ID:GNU,Clang,AppleClang>")
target_compile_options(luminari_build INTERFACE
    $<${LUMINARI_GNU_LIKE}:-Wall;-Wextra>
    $<$<AND:${LUMINARI_GNU_LIKE},$<BOOL:${LUMINARI_WERROR}>>:-Werror>
    $<$<AND:${LUMINARI_GNU_LIKE},$<BOOL:${DEVELOPER_MODE}>>:-Wshadow;-Wcast-qual;-Wwrite-strings;-Wconversion;-Wunreachable-code>)
target_compile_definitions(luminari_build INTERFACE
    $<$<BOOL:${MEMORY_DEBUG}>:MEMORY_DEBUG>
    $<$<BOOL:${DMALLOC}>:DMALLOC>)
if (LUMINARI_SANITIZERS)
    target_compile_options(luminari_build INTERFACE
        -fsanitize=${LUMINARI_SANITIZERS} -fno-omit-frame-pointer)
    target_link_options(luminari_build INTERFACE -fsanitize=${LUMINARI_SANITIZERS})
endif()
if (LUMINARI_COVERAGE)
    target_compile_options(luminari_build INTERFACE --coverage)
    target_link_options(luminari_build INTERFACE --coverage)
endif()
if (LUMINARI_HARDENING)
    target_compile_definitions(luminari_build INTERFACE
        $<$<NOT:$<CONFIG:Debug>>:_FORTIFY_SOURCE=3>)
    target_compile_options(luminari_build INTERFACE
        -fstack-protector-strong -fstack-clash-protection -fPIE)
    target_link_options(luminari_build INTERFACE
        -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack)
endif()

# ========== Source files ==========
# Main C source files
set(SRC_C_FILES
    src/account.c
    src/act.comm.c
    src/act.comm.do_spec_comm.c
    src/act.informative.c
    src/obj/act.item.c
    src/combat/act.offensive.c
    src/act.other.c
    src/ready_action.c
    src/tactical_effects.c
    src/act.social.c
    src/act.wizard.c
    src/actionqueues.c
    src/actions.c
    src/olc/aedit.c
    src/ai_cache.c
    src/ai_events.c
    src/ai_security.c
    src/ai_service.c
    src/craft/alchemy.c
    src/asciimap.c
    src/combat/assign_wpn_armor.c
    src/combat/projectiles.c
    src/combat/projectiles.h
    src/character/abilities.c
    src/character/backgrounds.c
    src/character/guild_services.c
    src/character/character_creation.c
    src/character/character_creation_content.c
    src/ban.c
    src/bardic_performance.c
    src/olc/bedit.c
    src/comms/boards.c
    src/craft/brew.c
    src/bsd-snprintf.c
    src/olc/cedit.c
    src/char_descs.c
    src/clan.c
    src/clan_services.c
    src/clan_economy.c
    src/olc/clan_edit.c
    src/clan_transactions.c
    src/character/class.c
    src/combat/combat_modes.c
    src/comm.c
    src/config.c
    src/constants.c
    src/copyover_diagnostic.c
    src/craft/craft.c
    src/craft/crafting_molds.c
    src/craft/crafting_new.c
    src/craft/crafting_recipes.c
    src/craft/crafts.c
    src/db.c
    src/db_init.c
    src/db_init_data.c
    src/db_startup_init.c
    src/db_admin_commands.c
    src/character/deities.c
    src/wilderness/desc_engine.c
    src/dgscript/dg_comm.c
    src/net/discord_bridge.c
    src/dgscript/dg_db_scripts.c
    src/dgscript/dg_event.c
    src/dgscript/dg_handler.c
    src/dgscript/dg_misc.c
    src/dgscript/dg_mobcmd.c
    src/dgscript/dg_objcmd.c
    src/dgscript/dg_olc.c
    src/dgscript/dg_scripts.c
    src/dgscript/dg_triggers.c
    src/dgscript/dg_variables.c
    src/dgscript/dg_wldcmd.c
    src/magic/domain_powers.c
    src/magic/domains_schools.c
    src/dotenv.c
    src/elf_build_id.c
    src/combat/encounters.c
    src/combat/combat_encounters.c
    src/combat/combat_damage.c
    src/combat/combat_damage.h
    src/combat/combat_death.h
    src/combat/combat_reactions.c
    src/combat/combat_reactions.h
    src/combat/combat_state.c
    src/combat/combat_state.h
    src/combat/combat_encounters.h
    src/character/evolutions.c
    src/character/feats.c
    src/combat/fight.c
    src/olc/genmob.c
    src/olc/genobj.c
    src/olc/genolc.c
    src/olc/genqst.c
    src/olc/genshp.c
    src/olc/genwld.c
    src/olc/genzon.c
    src/game_scheduler.c
    src/game_scheduler.h
    src/event_runtime.c
    src/event_runtime.h
    src/event_handle.h
    src/active_world.c
    src/active_world.h
    src/activity_manager.c
    src/activity_manager.h
    src/affected_owners.c
    src/affected_owners.h
    src/character_periodic.c
    src/character_periodic.h
    src/point_update_periodic.c
    src/point_update_periodic.h
    src/vessels/vessel_periodic.c
    src/vessels/vessel_periodic.h
    src/quest/staff_event_agenda.c
    src/quest/staff_event_agenda.h
    src/vessels/moving_room_events.c
    src/vessels/moving_room_events.h
    src/magic/buff_sequence.c
    src/magic/buff_sequence.h
    src/vessels/transport_jobs.c
    src/vessels/transport_jobs.h
    src/periodic_owners.c
    src/periodic_owners.h
    src/domain_events.c
    src/domain_events.h
    src/domain_event_types.c
    src/domain_event_world.c
    src/domain_event_world.h
    src/domain_event_types.h
    src/domain_object_transfer.c
    src/domain_object_transfer.h
    src/domain_event_runtime.c
    src/domain_event_runtime.h
    src/event_debug.c
    src/event_debug.h
    src/reactor.c
    src/reactor.h
    src/graph.c
    src/combat/grapple.c
    src/handler.c
    src/olc/hedit.c
    src/help.c
    src/helpers.c
    src/olc/hlqedit.c
    src/quest/hlquest.c
    src/obj/house.c
    src/olc/hsedit.c
    src/quest/hunts.c
    src/comms/ibt.c
    src/olc/improved-edit.c
    src/interpreter.c
    src/introduce.c
    src/wilderness/kdtree.c
    src/limits.c
    src/lists.c
    src/magic/magic.c
    src/comms/mail.c
    src/olc/medit.c
    src/magic/metamagic_science.c
    src/quest/missions.c
    src/mob/mob_act.c
    src/mob/mob_autoroll.c
    src/mob/mob_autoroll.h
    src/mob/mob_autoroll_profile.c
    src/mob/mob_memory.c
    src/mob/mob_utils.c
    src/mob/mob_race.c
    src/mob/mob_psionic.c
    src/mob/mob_class.c
    src/mob/mob_known_spells.c
    src/mob/mob_spells.c
    src/mob/mob_spellslots.c
    src/modify.c
    src/magic/moon_bonus_spells.c
    src/olc/msgedit.c
    src/mud_event.c
    src/mud_event_list.c
    src/mysql.c
    src/comms/mysql_boards.c
    src/comms/new_mail.c
    src/olc/oasis.c
    src/olc/oasis_copy.c
    src/olc/oasis_delete.c
    src/olc/oasis_list.c
    src/obj/objsave.c
    src/obj/player_shop.c
    src/olc/oedit.c
    src/password.c
    src/perfmon.c
    src/character/perks.c
    src/wilderness/perlin.c
    src/player_rename.c
    src/players.c
    src/olc/prefedit.c
    src/character/premadebuilds.c
    src/net/msdp_json.c
    src/net/msdp_json.h
    src/net/protocol.c
    src/magic/psionics.c
    src/olc/qedit.c
    src/quest/quest.c
    src/quest/quest_services.c
    src/rol_feats.c
    src/character/race.c
    src/random.c
    src/random_names.c
    src/rank.c
    src/olc/redit.c
    src/wilderness/resource_system.c
    src/wilderness/resource_depletion.c
    src/wilderness/resource_descriptions.c
    src/wilderness/resource_regeneration.c
    src/roleplay.c
    src/vessels/routing.c
    src/olc/sedit.c
    src/olc/spec_menu.c
    src/obj/shop.c
    src/combat/spec_abilities.c
    src/spec/spec_assign_table.c
    src/spec/spec_binding.c
    src/spec/spec_combat.c
    src/spec/spec_context.c
    src/spec/spec_cooldown.c
    src/spec/spec_dispatch.c
    src/spec/spec_effects.c
    src/spec/spec_effective_binding.c
    src/spec/spec_mobile_archetypes.c
    src/spec/spec_mobiles.c
    src/spec/spec_objects.c
    src/spec/spec_phrase.c
    src/spec/spec_rol_avernus.c
    src/spec/spec_rol_combat.c
    src/spec/spec_rol_darkhold.c
    src/spec/spec_rol_deaths_head.c
    src/spec/spec_rol_drow.c
    src/spec/spec_rol_residual_mobile.c
    src/spec/spec_rol_conversion.c
    src/spec/spec_rol_lavatubes.c
    src/spec/spec_rol_tarrasque.c
    src/spec/spec_rol_utility_objects.c
    src/spec/spec_rol_periodic_profiles.inc
    src/spec/spec_rol_state_periodic_profiles.inc
    src/spec/spec_rol_pilot.c
    src/spec/spec_rol_totem.c
    src/spec/spec_rooms.c
    src/spec/spec_zone_abyss.c
    src/spec/spec_zone_abyssal_vortex.c
    src/spec/spec_zone_agrach_dyrr.c
    src/spec/spec_zone_air_plane.c
    src/spec/spec_zone_alarm_group.c
    src/spec/spec_zone_bandit_castle.c
    src/spec/spec_zone_battlemaze.c
    src/spec/spec_zone_banshee.c
    src/spec/spec_zone_celestial_leviathan.c
    src/spec/spec_zone_crimson_flame.c
    src/spec/spec_zone_earth_plane.c
    src/spec/spec_zone_feybranche.c
    src/spec/spec_zone_fire_giant.c
    src/spec/spec_zone_flaming_tower.c
    src/spec/spec_zone_hive_of_passion.c
    src/spec/spec_zone_illithid_enclave.c
    src/spec/spec_zone_jot.c
    src/spec/spec_zone_kenjin_tower.c
    src/spec/spec_zone_kings_castle.c
    src/spec/spec_zone_kobold_caverns.c
    src/spec/spec_zone_longsaddle.c
    src/spec/spec_zone_mad_drow.c
    src/spec/spec_zone_mere_of_dead_men.c
    src/spec/spec_zone_menzoberranzan.c
    src/spec/spec_zone_neverwinter.c
    src/spec/spec_zone_orc_ruins.c
    src/spec/spec_zone_prisoner.c
    src/spec/spec_zone_quicksand.c
    src/spec/spec_zone_secomber.c
    src/spec/spec_zone_shadow_dragon.c
    src/spec/spec_zone_shobalar.c
    src/spec/spec_zone_ttf.c
    src/spec/spec_zone_zusuk.c
    src/spec/spec_registry.c
    src/spec/spec_assign.c
    src/spec/spec_assign_mobiles.c
    src/spec/spec_assign_objects.c
    src/spec/spec_assign_rooms.c
    src/obj/spec_artifacts.c
    src/magic/casting_visuals.c
    src/magic/spell_parser.c
    src/magic/spell_prep.c
    src/magic/spell_lists.c
    src/magic/spellbook_scroll.c
    src/magic/spells.c
    src/quest/staff_events.c
    src/character/skill_lists.c
    src/character/study.c
    src/character/talents.c
    src/olc/tedit.c
    src/character/templates.c
    src/obj/trade.c
    src/obj/vendor.c
    src/character/vampire_cloak.c
    src/vessels/transport.c
    src/vessels/transport_unified.c
    src/combat/traps_new.c
    src/obj/treasure.c
    src/obj/treasure_const.c
    src/utils.c
    src/vessels/vehicles.c
    src/vessels/vehicles_commands.c
    src/vessels/vehicles_transport.c
    src/vessels/vessels.c
    src/vessels/vessels_legacy.c
    src/vessels/vessels_rol.c
    src/vessels/vessels_moving_rooms.c
    src/vessels/vessels_tactical.c
    src/vessels/vessels_lookout.c
    src/vessels/vessels_narrative.c
    src/vessels/vessels_autopilot.c
    src/vessels/vessels_admin.c
    src/vessels/vessels_balance.c
    src/vessels/vessels_combat.c
    src/vessels/vessels_crew.c
    src/vessels/vessels_hazards.c
    src/vessels/vessels_hunters.c
    src/vessels/vessels_merchants.c
    src/vessels/vessels_events.c
    src/vessels/vessels_db.c
    src/vessels/vessels_docking.c
    src/vessels/vessels_edit.c
    src/vessels/vessels_ownership.c
    src/vessels/vessels_rooms.c
    src/vessels/vessels_contracts.c
    src/vessels/vessels_piracy.c
    src/vessels/vessels_trade.c
    src/vessels/vessels_upgrades.c
    src/weather.c
    src/wilderness/wilderness.c
    src/wilderness/wilderness_kb.c
    src/olc/zedit.c
    src/zmalloc.c
    src/movement/movement.c
    src/movement/movement_validation.c
    src/movement/movement_cost.c
    src/movement/movement_position.c
    src/movement/door_state.c
    src/movement/movement_doors.c
    src/movement/movement_falling.c
    src/movement/movement_messages.c
    src/movement/movement_tracks.c
    src/movement/movement_events.c
    src/wilderness/spatial_core.c
    src/wilderness/spatial_visual.c
    src/wilderness/spatial_audio.c
    src/wilderness/spatial_events.c
    src/wilderness/spatial_events.h
    src/mob/phenomenon_response.c
    src/mob/phenomenon_response.h
    src/wilderness/terrain_bridge.c
    src/wilderness/region_hints.c
    src/wilderness/narrative_weaver.c
    src/net/onboarding.c
    src/net/i3_client.c
    src/net/i3_commands.c
    src/net/i3_utils.c
    src/pet_vnums.h
    src/elf_build_id.h
    src/player_rename.h
    src/tactical_effects.h
)

set(SRC_FILES ${SRC_C_FILES} ${BUILD_IDENTITY_HEADER})
set_source_files_properties(${BUILD_IDENTITY_HEADER} PROPERTIES GENERATED TRUE)

# ========== Build main executable FIRST ==========
add_executable(luminari ${SRC_FILES})
add_custom_target(refresh_build_identity ALL
    COMMAND bash
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/deployment/generate_build_identity.sh
            ${BUILD_IDENTITY_HEADER}
            ${CMAKE_CURRENT_SOURCE_DIR}
    BYPRODUCTS ${BUILD_IDENTITY_HEADER}
    COMMENT "Refreshing embedded Git build identity")
add_dependencies(luminari refresh_build_identity)

target_link_libraries(luminari PRIVATE luminari_build luminari_deps)

# ========== Build utilities AFTER main executable ==========
# Build utilities by default (can be disabled with -DBUILD_UTILS=OFF)
if (BUILD_UTILS)
    # Utilities from util/ directory
    add_executable(autowiz util/autowiz.c)
    add_executable(asciipasswd util/asciipasswd.c)
    add_executable(plrtoascii util/plrtoascii.c)
    add_executable(rebuildAsciiIndex util/rebuildAsciiIndex.c)
    add_executable(rebuildMailIndex util/rebuildMailIndex.c)
    add_executable(rol_mob_calculator util/rol_mob_calculator.c
                                      src/mob/mob_autoroll.c
                                      src/mob/mob_autoroll_profile.c)
    add_executable(shopconv util/shopconv.c)
    add_executable(sign util/sign.c)
    add_executable(split util/split.c)
    add_executable(wld2html util/wld2html.c)

    set(LUMINARI_UTIL_TARGETS autowiz asciipasswd plrtoascii rebuildAsciiIndex
        rebuildMailIndex rol_mob_calculator shopconv sign split wld2html)
    set_target_properties(${LUMINARI_UTIL_TARGETS}
                          PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})
    foreach(UTIL_TARGET ${LUMINARI_UTIL_TARGETS})
        target_link_libraries(${UTIL_TARGET} PRIVATE luminari_build)
    endforeach()
    target_link_libraries(asciipasswd PRIVATE ${CRYPT_LIBRARY})
endif()

# ========== Standalone world-data tools ==========
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)

# Keep this list synchronized with world_tool_sources in Makefile.am.
set(WORLD_TOOL_SOURCES
    scripts/world/install_pet_constructs.py
    data/pet-constructs/196.mob
    data/pet-planar-allies/197.mob
    data/pet-undead/198.mob
    data/pet-lycanthropes/195.mob
    data/pet-lycanthropes/195.zon
    data/pet-illusions/199.mob
    data/pet-illusions/199.zon
    data/pet-undead/198.zon
    data/pet-undead/README.md
    data/pet-planar-allies/197.zon
    data/pet-planar-allies/README.md
    data/pet-constructs/README.md
    data/pet-constructs/196.zon
    scripts/world/generate_rol_periodic_profiles.py
    scripts/world/generate_rol_state_periodic_profiles.py
    scripts/world/wtool.py
    scripts/world/wtool_constants.json
    scripts/world/rol_conversion_policy.json
    scripts/world/rol_source_race_registry.json
    scripts/world/wtool_lib/__init__.py
    scripts/world/wtool_lib/cli.py
    scripts/world/wtool_lib/config.py
    scripts/world/wtool_lib/constants.py
    scripts/world/wtool_lib/docs_check.py
    scripts/world/wtool_lib/flags.py
    scripts/world/wtool_lib/hlquests.py
    scripts/world/wtool_lib/indexes.py
    scripts/world/wtool_lib/lookup.py
    scripts/world/wtool_lib/models.py
    scripts/world/wtool_lib/mobiles.py
    scripts/world/wtool_lib/objects.py
    scripts/world/wtool_lib/parsing.py
    scripts/world/wtool_lib/quests.py
    scripts/world/wtool_lib/reporting.py
    scripts/world/wtool_lib/rol_baseline.py
    scripts/world/wtool_lib/rol_capability_audit.py
    scripts/world/wtool_lib/rol_discovery.py
    scripts/world/wtool_lib/rol_graph.py
    scripts/world/wtool_lib/rol_inventory.py
    scripts/world/wtool_lib/rol_json.py
    scripts/world/wtool_lib/rol_mob_calculator.py
    scripts/world/wtool_lib/rol_mobile_identity.py
    scripts/world/wtool_lib/rol_identity.py
    scripts/world/wtool_lib/rol_persistence.py
    scripts/world/wtool_lib/rol_persistence_check.py
    scripts/world/wtool_lib/rol_planner.py
    scripts/world/wtool_lib/rol_phase7.py
    scripts/world/wtool_lib/rol_phase8.py
    scripts/world/wtool_lib/rol_pilot.py
    scripts/world/wtool_lib/rol_pilot_build.py
    scripts/world/wtool_lib/rol_periodic_profiles.py
    scripts/world/wtool_lib/rol_skeleton.py
    scripts/world/wtool_lib/rol_state_periodic_profiles.py
    scripts/world/wtool_lib/rol_soc.py
    scripts/world/wtool_lib/rol_special.py
    scripts/world/wtool_lib/rol_conversion_types.py
    scripts/world/wtool_lib/rol_source_common.py
    scripts/world/wtool_lib/rol_transform_common.py
    scripts/world/wtool_lib/rol_mobiles.py
    scripts/world/wtool_lib/rol_objects.py
    scripts/world/wtool_lib/rol_armor_mapping.py
    scripts/world/wtool_lib/rol_armor_overrides.json
    scripts/world/wtool_lib/rol_weapon_overrides.json
    scripts/world/wtool_lib/rol_rooms.py
    scripts/world/wtool_lib/rol_zones.py
    scripts/world/wtool_lib/rol_shops.py
    scripts/world/wtool_lib/rol_quests.py
    scripts/world/wtool_lib/rol_source.py
    scripts/world/wtool_lib/rol_transform.py
    scripts/world/wtool_lib/rooms.py
    scripts/world/wtool_lib/semantics.py
    scripts/world/wtool_lib/shops.py
    scripts/world/wtool_lib/source.py
    scripts/world/wtool_lib/spec_registry.py
    scripts/world/wtool_lib/triggers.py
    scripts/world/wtool_lib/world.py
    scripts/world/wtool_lib/zones.py
    scripts/world/tests/__init__.py
    scripts/world/tests/test_cli.py
    scripts/world/tests/test_constants.py
    scripts/world/tests/test_docs_check.py
    scripts/world/tests/test_graph.py
    scripts/world/tests/test_hlquests.py
    scripts/world/tests/test_indexes.py
    scripts/world/tests/test_lookup.py
    scripts/world/tests/test_mobiles.py
    scripts/world/tests/test_objects.py
    scripts/world/tests/test_quests.py
    scripts/world/tests/test_reporting.py
    scripts/world/tests/test_rol_baseline.py
    scripts/world/tests/rol_reference.py
    scripts/world/tests/test_rol_capability_audit.py
    scripts/world/tests/test_rol_discovery.py
    scripts/world/tests/test_rol_graph.py
    scripts/world/tests/test_rol_inventory.py
    scripts/world/tests/test_rol_mob_calculator.py
    scripts/world/tests/test_rol_mobile_identity.py
    scripts/world/tests/test_rol_identity.py
    scripts/world/tests/test_rol_persistence_check.py
    scripts/world/tests/test_rol_planner.py
    scripts/world/tests/test_rol_phase7.py
    scripts/world/tests/test_rol_phase8.py
    scripts/world/tests/test_rol_pilot.py
    scripts/world/tests/test_rol_pilot_build.py
    scripts/world/tests/test_rol_periodic_profiles.py
    scripts/world/tests/test_rol_skeleton.py
    scripts/world/tests/test_rol_state_periodic_profiles.py
    scripts/world/tests/test_rol_source.py
    scripts/world/tests/test_rol_transform.py
    scripts/world/tests/test_rooms.py
    scripts/world/tests/test_semantics.py
    scripts/world/tests/test_shops.py
    scripts/world/tests/test_source.py
    scripts/world/tests/test_triggers.py
    scripts/world/tests/test_wrapper.py
    scripts/world/tests/test_zones.py
    scripts/world/tests/fixtures/rol_inventory/malformed/areas/AREA
    scripts/world/tests/fixtures/rol_inventory/malformed/areas/AREA.mobobj
    scripts/world/tests/fixtures/rol_inventory/malformed/areas/QUEST
    scripts/world/tests/fixtures/rol_inventory/malformed/areas/SHOP
    scripts/world/tests/fixtures/rol_inventory/valid/areas/AREA
    scripts/world/tests/fixtures/rol_inventory/valid/areas/AREA.mobobj
    scripts/world/tests/fixtures/rol_inventory/valid/areas/QUEST
    scripts/world/tests/fixtures/rol_inventory/valid/areas/SHOP
    scripts/world/tests/fixtures/rol_inventory/valid/areas/mob/alpha.mob
    scripts/world/tests/fixtures/rol_inventory/valid/areas/mob/disabled.mob
    scripts/world/tests/fixtures/rol_inventory/valid/areas/mob/orphan.mob
    scripts/world/tests/fixtures/rol_inventory/valid/areas/obj/alpha.obj
    scripts/world/tests/fixtures/rol_inventory/valid/areas/obj/companion.obj
    scripts/world/tests/fixtures/rol_inventory/valid/areas/obj/disabled.obj
    scripts/world/tests/fixtures/rol_inventory/valid/areas/obj/orphan.obj
    scripts/world/tests/fixtures/rol_inventory/valid/areas/qst/alpha.qst
    scripts/world/tests/fixtures/rol_inventory/valid/areas/qst/companion.qst
    scripts/world/tests/fixtures/rol_inventory/valid/areas/qst/disabled.qst
    scripts/world/tests/fixtures/rol_inventory/valid/areas/shp/alpha.shp
    scripts/world/tests/fixtures/rol_inventory/valid/areas/soc/alpha.soc
    scripts/world/tests/fixtures/rol_inventory/valid/areas/wld/alpha.wld
    scripts/world/tests/fixtures/rol_inventory/valid/areas/wld/disabled.wld
    scripts/world/tests/fixtures/rol_inventory/valid/areas/wld/orphan.wld
    scripts/world/tests/fixtures/rol_inventory/valid/areas/wld/split.wld
    scripts/world/tests/fixtures/rol_inventory/valid/areas/zon/alpha.zon
    scripts/world/tests/fixtures/rol_inventory/valid/areas/zon/disabled.zon
    scripts/world/tests/fixtures/rol_inventory/valid/areas/zon/orphan.zon
    scripts/world/tests/fixtures/rol_inventory/valid/areas/zon/split.zon
    scripts/world/tests/fixtures/phase1/valid/100.zon
    scripts/world/tests/fixtures/phase1/valid/100.wld
    scripts/world/tests/fixtures/phase1/broken/diagonal.wld
    scripts/world/tests/fixtures/phase1/broken/header-truncation.zon
    scripts/world/tests/fixtures/phase1/broken/invalid-directions.wld
    scripts/world/tests/fixtures/phase1/broken/invalid-directions.zon
    scripts/world/tests/fixtures/phase1/broken/moving-limit.wld
    scripts/world/tests/fixtures/phase1/broken/out-of-order.wld
    scripts/world/tests/fixtures/phase1/broken/prescan.zon
    scripts/world/tests/fixtures/phase1/broken/reset-traps.zon
    scripts/world/tests/fixtures/phase1/broken/room-structure.wld
    scripts/world/tests/fixtures/phase2/complete/hlq/100.hlq
    scripts/world/tests/fixtures/phase2/complete/hlq/index
    scripts/world/tests/fixtures/phase2/complete/hlq/index.mini
    scripts/world/tests/fixtures/phase2/complete/mob/100.mob
    scripts/world/tests/fixtures/phase2/complete/mob/index
    scripts/world/tests/fixtures/phase2/complete/mob/index.mini
    scripts/world/tests/fixtures/phase2/complete/obj/100.obj
    scripts/world/tests/fixtures/phase2/complete/obj/index
    scripts/world/tests/fixtures/phase2/complete/obj/index.mini
    scripts/world/tests/fixtures/phase2/complete/qst/100.qst
    scripts/world/tests/fixtures/phase2/complete/qst/index
    scripts/world/tests/fixtures/phase2/complete/qst/index.mini
    scripts/world/tests/fixtures/phase2/complete/shp/100.shp
    scripts/world/tests/fixtures/phase2/complete/shp/index
    scripts/world/tests/fixtures/phase2/complete/shp/index.mini
    scripts/world/tests/fixtures/phase2/complete/shp/legacy.shp
    scripts/world/tests/fixtures/phase2/complete/trg/100.trg
    scripts/world/tests/fixtures/phase2/complete/trg/index
    scripts/world/tests/fixtures/phase2/complete/trg/index.mini
    scripts/world/tests/fixtures/phase2/complete/wld/100.wld
    scripts/world/tests/fixtures/phase2/complete/wld/index
    scripts/world/tests/fixtures/phase2/complete/wld/index.mini
    scripts/world/tests/fixtures/phase2/complete/zon/100.zon
    scripts/world/tests/fixtures/phase2/complete/zon/index
    scripts/world/tests/fixtures/phase2/complete/zon/index.mini
    scripts/world/tests/fixtures/phase2/contracts/hlq/canonical.hlq
    scripts/world/tests/fixtures/phase2/contracts/lookup/existing-six.json
    scripts/world/tests/fixtures/phase2/contracts/qst/canonical.qst
    scripts/world/tests/fixtures/phase2/contracts/qst/legacy.qst)

# Keep this list synchronized with world_tool_support_files in Makefile.am.
set(WORLD_TOOL_SUPPORT_FILES
    lib/world/validate-zone.sh
    lib/world/minimal/0.mob
    lib/world/minimal/0.obj
    lib/world/minimal/0.wld
    lib/world/minimal/0.zon
    lib/world/minimal/16.mob
    scripts/development/generate-web-guides.sh
    docs/guides/OLC_SpecProcs.md
    docs/systems/OLC_ONLINE_CREATION_SYSTEM.md
    docs/utilities/WORLD_VALIDATOR_CLI.md
    docs/web/assets/pandoc-template.html
    docs/web/guides/mob_flags.html
    docs/web/guides/oedit.html
    docs/web/guides/room_flags.html
    docs/world/STARTER_AREA.md
    docs/world_game-data/BUILDER_QUICKSTART.md
    docs/world_game-data/CRAFTING_SYSTEM_NOTES.md
    docs/world_game-data/MOB_FLAGS.md
    docs/world_game-data/HLQUEST_FILE_FORMAT.md
    docs/world_game-data/OEDIT_GUIDE.md
    docs/world_game-data/QUEST_FILE_FORMAT.md
    docs/world_game-data/ROOM_FLAGS.md
    docs/world_game-data/SHOP_FILE_FORMAT.md
    docs/world_game-data/ZONE_FILE_FORMAT.md
    docs/world_game-data/builder_manual.md
    docs/world_game-data/gear_guide.md
    docs/world_game-data/wilderness_system.md)

add_custom_target(check-world-docs
    COMMAND Python3::Interpreter scripts/world/wtool.py constants sync --check
    COMMAND Python3::Interpreter scripts/world/wtool.py docs --check
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    SOURCES ${WORLD_TOOL_SOURCES} ${WORLD_TOOL_SUPPORT_FILES}
    COMMENT "Checking world-data constants and documentation")

set(WORLD_TOOL_CALCULATOR_ENV)
if (TARGET rol_mob_calculator)
    set(WORLD_TOOL_CALCULATOR_ENV
        "ROL_MOB_CALCULATOR=$<TARGET_FILE:rol_mob_calculator>")
endif()

add_custom_target(test-world-tools
    COMMAND ${CMAKE_COMMAND} -E env ${WORLD_TOOL_CALCULATOR_ENV}
            ${Python3_EXECUTABLE} -m unittest discover
            -s scripts/world/tests -t scripts/world -v
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/lib/world/validate-zone.sh 100
            --world-root
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/world/tests/fixtures/phase2/complete
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    SOURCES ${WORLD_TOOL_SOURCES} ${WORLD_TOOL_SUPPORT_FILES}
    COMMENT "Running standalone world-data tool tests")
add_dependencies(test-world-tools check-world-docs)
if (TARGET rol_mob_calculator)
    add_dependencies(test-world-tools rol_mob_calculator)
endif()

set(HELP_SYNC_SOURCES
    .agents/skills/help-sync/SKILL.md
    .agents/skills/help-sync/agents/openai.yaml
    scripts/help-sync/catalog.py
    scripts/help-sync/endpoint.py
    scripts/help-sync/help_sync.py
    scripts/help-sync/requirements.txt
    scripts/help-sync/tests/test_catalog.py
    scripts/help-sync/tests/test_endpoint.py
    scripts/help-sync/tests/test_endpoint_integration.py
    sql/components/help_sync_schema.sql
    sql/components/verify_help_sync_schema.sql)

add_custom_target(test-help-sync
    COMMAND ${Python3_EXECUTABLE} -m unittest discover
            -s scripts/help-sync/tests -p test_*.py -v
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    SOURCES ${HELP_SYNC_SOURCES}
    COMMENT "Running help synchronization unit tests")

add_custom_target(test-help-sync-integration
    COMMAND ${CMAKE_COMMAND} -E env LUMINARI_HELP_SYNC_INTEGRATION=1
            ${Python3_EXECUTABLE} -m unittest
            scripts/help-sync/tests/test_endpoint_integration.py -v
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    SOURCES ${HELP_SYNC_SOURCES}
    COMMENT "Running isolated MariaDB help synchronization tests")

# ========== Build unit tests AFTER main executable ==========
# The focused protocol parser harness remains available under unittests/CuTest.

# Keep this list synchronized with cutest_test_files in Makefile.am.
set(CUTEST_TEST_SOURCES
    unittests/CuTest/test_syntax_check_boot.c
    unittests/CuTest/test_web_onboarding.c
    unittests/CuTest/test_race_equivalence.c
    unittests/CuTest/test_i3_client_production.c
    unittests/CuTest/test_msdp_production.c
    unittests/CuTest/test_necromancer.c
    unittests/CuTest/test_pet_persistence.c
    unittests/CuTest/test_pet_policy.c
    unittests/CuTest/test_pet_lifetime.c
    unittests/CuTest/test_study_pet_menus.c
    unittests/CuTest/test_bounds_checking.c
    unittests/CuTest/test_help_sync.c
    unittests/CuTest/test_artifacts.c
    unittests/CuTest/test_artifact_integration.c
    unittests/CuTest/test_player_rename.c
    unittests/CuTest/test_ai_secrets.c
    unittests/CuTest/test.helpers.c
    unittests/CuTest/test.interpreter.c
    unittests/CuTest/test_transport_production.c
    unittests/CuTest/test_traps_production.c
    unittests/CuTest/test_combat_production.c
    unittests/CuTest/test_combat_encounters.c
    unittests/CuTest/test_activity_manager.c
    unittests/CuTest/test_thrown_weapons.c
    unittests/CuTest/test_lists_production.c
    unittests/CuTest/test_mob_autoroll.c
    unittests/CuTest/test_copyover_timer.c
    unittests/CuTest/test_reactor.c
    unittests/CuTest/test_game_scheduler.c
    unittests/CuTest/test_event_runtime.c
    unittests/CuTest/test_door_events.c
    unittests/CuTest/test_domain_events.c
    unittests/CuTest/test_native_event_integration.c
    unittests/CuTest/test_bardic_performance.c
    unittests/CuTest/test_rol_feats.c
    unittests/CuTest/test_unassigned_spells.c
    unittests/CuTest/test_spells_skills_production.c
    unittests/CuTest/test_perfmon_production.c
    unittests/CuTest/test_elf_build_id.c
    unittests/CuTest/test_vessel_events.c
    unittests/CuTest/test_vessel_tactical.c
    unittests/CuTest/test_vessel_lookout.c
    unittests/CuTest/test_vessel_narrative.c
    unittests/CuTest/test_vessel_boarding.c
    unittests/CuTest/test_database_persistence.c
    unittests/CuTest/test_dg_scripts_production.c
    unittests/CuTest/test_ashenport_welcome_trigger.c
    unittests/CuTest/test_damage_trigger.c
    unittests/CuTest/test_upstream_regressions.c
    unittests/CuTest/test_world_loading_production.c
    unittests/CuTest/test_spec_fixtures.c
    unittests/CuTest/test_spec_authored_bindings.c
    unittests/CuTest/test_spec_binding_round_trip.c
    unittests/CuTest/test_spec_effective_binding.c
    unittests/CuTest/test_spec_registry_persistence.c
    unittests/CuTest/test_spec_registry_validation.c
    unittests/CuTest/test_spec_owner_aware_olc.c
    unittests/CuTest/test_spec_command_pulse.c
    unittests/CuTest/test_spec_assign_table.c
    unittests/CuTest/test_spec_dispatch.c
    unittests/CuTest/test_spec_mechanics.c
    unittests/CuTest/test_spec_typed_handlers.c
    unittests/CuTest/test_spec_combat_secondary.c
    unittests/CuTest/test_gameplay_e2e.c
    unittests/CuTest/test_password.c
)

# The focused protocol harnesses are built by unittests/CuTest/Makefile.
set(PROTOCOL_FOCUSED_TEST_SOURCES
    unittests/CuTest/test_protocol_parser.c
    unittests/CuTest/fuzz_protocol_parser.c
)

if (BUILD_TESTS)
    enable_testing()

    add_executable(bsd_snprintf_fallback_test src/bsd-snprintf.c)
    target_compile_definitions(bsd_snprintf_fallback_test PRIVATE
        BROKEN_SNPRINTF
        TEST_SNPRINTF)
    target_link_libraries(bsd_snprintf_fallback_test PRIVATE luminari_build m)
    add_test(NAME bsd-snprintf-fallback COMMAND bsd_snprintf_fallback_test)

    set(CUTEST_GENERATOR
        ${CMAKE_CURRENT_SOURCE_DIR}/unittests/CuTest/make-tests.sh)
    set(CUTEST_ALL_TESTS_SOURCE
        ${CMAKE_CURRENT_BINARY_DIR}/AllTests.c)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        ${CUTEST_GENERATOR}
        ${CUTEST_TEST_SOURCES})

    execute_process(
        COMMAND ${CUTEST_GENERATOR} ${CUTEST_TEST_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_FILE ${CUTEST_ALL_TESTS_SOURCE}
        RESULT_VARIABLE CUTEST_GENERATOR_RESULT)
    if (NOT CUTEST_GENERATOR_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to generate the CuTest test registry")
    endif()

    add_executable(cutest
        unittests/CuTest/CuTest.c
        ${CUTEST_TEST_SOURCES}
        ${CUTEST_ALL_TESTS_SOURCE}
        ${SRC_FILES})
    add_dependencies(cutest refresh_build_identity)
    set_target_properties(cutest PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    target_include_directories(cutest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/unittests/CuTest)
    target_compile_definitions(cutest PRIVATE LUMINARI_CUTEST)
    target_link_libraries(cutest PRIVATE luminari_build luminari_deps)

    add_test(NAME build-parity
        COMMAND Python3::Interpreter scripts/ci/check_build_parity.py)
    add_test(NAME sql-interpolation-self-test
        COMMAND Python3::Interpreter scripts/ci/check_sql_interpolation.py --self-test)
    add_test(NAME sql-interpolation
        COMMAND Python3::Interpreter scripts/ci/check_sql_interpolation.py)
    add_test(NAME help-sync
        COMMAND Python3::Interpreter -m unittest discover
                -s scripts/help-sync/tests -p test_*.py -v)
    add_test(NAME background-help-entries
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_background_help_entries.sh)
    set_tests_properties(
        build-parity
        sql-interpolation-self-test
        sql-interpolation
        help-sync
        background-help-entries
        PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    add_test(NAME production-cutest COMMAND cutest)
    set_tests_properties(production-cutest PROPERTIES
        ENVIRONMENT
            "LUMINARI_TEST_ROOT=${CMAKE_CURRENT_SOURCE_DIR};LUMINARI_TEST_SPEC_WORLD_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/unittests/CuTest/fixtures/spec_world_inventory"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    add_test(NAME demand-driven-event-architecture
        COMMAND bash
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/events/test_demand_driven_architecture.sh)
    add_test(NAME event-core-performance-gate
        COMMAND Python3::Interpreter -m unittest
            scripts/events/test_event_core_performance_gate.py -v)
    add_test(NAME legacy-event-admission
        COMMAND bash
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/events/test_legacy_event_admission.sh)
    add_test(NAME native-event-architecture
        COMMAND bash
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/events/test_native_event_architecture.sh)
    add_test(NAME event-pubsub-retirement
        COMMAND bash
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/events/test_pubsub_retirement.sh)
    set_tests_properties(
        demand-driven-event-architecture
        event-core-performance-gate
        legacy-event-admission
        native-event-architecture
        event-pubsub-retirement
        PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    add_test(NAME world-tools
        COMMAND Python3::Interpreter -m unittest discover
                -s scripts/world/tests -t scripts/world -v)
    add_test(NAME world-tool-constants
        COMMAND Python3::Interpreter scripts/world/wtool.py constants sync --check)
    add_test(NAME world-tool-docs
        COMMAND Python3::Interpreter scripts/world/wtool.py docs --check)
    add_test(NAME world-tool-wrapper
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/lib/world/validate-zone.sh 100
                --world-root
                ${CMAKE_CURRENT_SOURCE_DIR}/scripts/world/tests/fixtures/phase2/complete)
    set_tests_properties(
        world-tools
        world-tool-constants
        world-tool-docs
        world-tool-wrapper
        PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    if (TARGET rol_mob_calculator)
        set_tests_properties(world-tools PROPERTIES
            ENVIRONMENT "ROL_MOB_CALCULATOR=$<TARGET_FILE:rol_mob_calculator>")
    endif()

    add_test(NAME autorun-supervision
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/autorun/test_autorun_supervision.sh)
    set_tests_properties(autorun-supervision PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME versioned-binary-install
        COMMAND
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/deployment/test_versioned_binary_install.sh)
    set_tests_properties(versioned-binary-install PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME binary-name-static
        COMMAND bash
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/deployment/test_binary_name_static.sh)
    set_tests_properties(binary-name-static PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    add_test(NAME healthcheck
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/operations/test_healthcheck.sh)
    set_tests_properties(healthcheck PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    add_test(NAME vessel-memory-analyzer
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/vessels/test_vessel_memory_analyzer.sh)
    add_test(NAME vessel-process-memory-details
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/process-memory/test_process_memory_details.sh)
    add_test(NAME process-memory-monitor
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/process-memory/test_monitor_process_memory.sh)
    add_test(NAME vessel-ferry-soak-preflight
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/vessels/test_vessel_ferry_soak_preflight.sh)
    add_test(NAME vessel-scale-benchmark-parsers
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/vessels/test_vessel_scale_benchmark_parsers.sh)
    set_tests_properties(
        vessel-memory-analyzer
        vessel-process-memory-details
        process-memory-monitor
        vessel-ferry-soak-preflight
        vessel-scale-benchmark-parsers
        PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

add_custom_target(test-autorun-supervision
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/autorun/test_autorun_supervision.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running autorun supervision regression tests"
)

add_custom_target(test-process-memory
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/process-memory/test_process_memory_details.sh
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/process-memory/test_monitor_process_memory.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running process memory regression tests"
)

add_custom_target(test-versioned-binary-install
    COMMAND
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/deployment/test_versioned_binary_install.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running versioned binary installation regression test"
)

add_custom_target(test-healthcheck
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/operations/test_healthcheck.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running healthcheck regression tests"
)

# ========== Static analysis ==========
# Opt-in so normal builds are reproducible regardless of whether clang-tidy is
# installed. The analyzer is attached to each executable target rather than
# set through the directory-wide CMAKE_C_CLANG_TIDY variable.
if (STATIC_ANALYSIS)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if (NOT CLANG_TIDY_EXE)
        message(FATAL_ERROR "STATIC_ANALYSIS requested but clang-tidy not found")
    endif()
    message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    get_directory_property(LUMINARI_ALL_TARGETS BUILDSYSTEM_TARGETS)
    foreach(ANALYZED_TARGET IN LISTS LUMINARI_ALL_TARGETS)
        get_target_property(ANALYZED_TYPE ${ANALYZED_TARGET} TYPE)
        if (ANALYZED_TYPE STREQUAL "EXECUTABLE")
            set_target_properties(${ANALYZED_TARGET} PROPERTIES
                C_CLANG_TIDY "${CLANG_TIDY_EXE}")
        endif()
    endforeach()
endif()

# ========== Installation ==========
install(CODE "
    execute_process(
        COMMAND
            \"${CMAKE_CURRENT_SOURCE_DIR}/scripts/deployment/install_versioned_binary.sh\"
            \"$<TARGET_FILE:luminari>\"
            \"${INSTALL_BIN_OUTPUT_DIR}\"
        RESULT_VARIABLE versioned_install_result)
    if (NOT versioned_install_result EQUAL 0)
        message(FATAL_ERROR \"Versioned luminari installation failed\")
    endif()
    execute_process(
        COMMAND \"${CMAKE_CURRENT_SOURCE_DIR}/scripts/provision_artifacts.sh\"
        RESULT_VARIABLE artifact_provision_result)
    if (NOT artifact_provision_result EQUAL 0)
        message(FATAL_ERROR \"Artifact world data provisioning failed\")
    endif()
")
if (BUILD_UTILS)
    install(TARGETS autowiz asciipasswd plrtoascii rebuildAsciiIndex
                    rebuildMailIndex shopconv sign split wld2html
            DESTINATION ${INSTALL_BIN_OUTPUT_DIR})
endif()

# ========== Summary ==========
message(STATUS "========================================")
message(STATUS "LuminariMUD CMake Configuration Summary:")
message(STATUS "  C Standard: GNU C23")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  MySQL Client: ${LUMINARI_MYSQL_TARGET}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Utilities: ${BUILD_UTILS}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
message(STATUS "  Developer Mode: ${DEVELOPER_MODE}")
message(STATUS "  Warnings As Errors: ${LUMINARI_WERROR}")
message(STATUS "  Sanitizers: ${LUMINARI_SANITIZERS}")
message(STATUS "  Coverage: ${LUMINARI_COVERAGE}")
message(STATUS "  Hardening: ${LUMINARI_HARDENING}")
message(STATUS "  Install Binary Output: ${INSTALL_BIN_OUTPUT_DIR}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Binary Output: ${BIN_OUTPUT_DIR}")
message(STATUS "========================================")

# Optional sound-pack source and distribution files; no runtime generation required.
add_custom_target(msp_sound_assets SOURCES
    scripts/development/generate_msp_sounds.py
    lib/sounds/README.md
    lib/sounds/luminari-test.wav
    lib/sounds/luminari-door-open.wav
    sql/components/help_screen_reader_msp.sql
)
