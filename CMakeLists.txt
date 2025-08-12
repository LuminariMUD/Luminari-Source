cmake_minimum_required(VERSION 3.12)
project(LuminariMUD C CXX)

# IMPORTANT: LuminariMUD uses ANSI C90/C89 standard, NOT C99!
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)  # For performance monitoring (perfmon.cpp)

# Include checker modules
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckTypeSize)
include(CheckStructHasMember)
include(CheckSymbolExists)
include(CheckCSourceCompiles)

# Output paths - LuminariMUD uses bin/circle convention
set(BIN_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})

# Include source paths
include_directories(src)

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

# ========== Compiler flags ==========
if (CMAKE_COMPILER_IS_GNUCC)
    include(CheckCCompilerFlag)

    check_c_compiler_flag(-Wall SUPPORTS_WALL)
    check_c_compiler_flag(-Wno-char-subscripts SUPPORTS_WNO_CHAR_SUBSCRIPTS)

    if (SUPPORTS_WALL)
        set(MYFLAGS "-Wall")
        if (SUPPORTS_WNO_CHAR_SUBSCRIPTS)
            set(MYFLAGS "${MYFLAGS} -Wno-char-subscripts")
        endif()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MYFLAGS}")
    endif()
endif()

# Static analysis with clang-tidy (enabled by default if available)
option(STATIC_ANALYSIS "Enable static analysis with clang-tidy" ON)

# clang-tidy if available
find_program(CLANG_TIDY_EXE NAMES clang-tidy)

if(CLANG_TIDY_EXE AND STATIC_ANALYSIS)
    message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
    set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_EXE}")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
elseif(STATIC_ANALYSIS)
    message(WARNING "STATIC_ANALYSIS requested but clang-tidy not found")
endif()

# ========== Header checks ==========
check_include_file("fcntl.h" HAVE_FCNTL_H)
check_include_file("errno.h" HAVE_ERRNO_H)
check_include_file("string.h" HAVE_STRING_H)
check_include_file("strings.h" HAVE_STRINGS_H)
check_include_file("limits.h" HAVE_LIMITS_H)
check_include_file("sys/select.h" HAVE_SYS_SELECT_H)
check_include_file("sys/wait.h" HAVE_SYS_WAIT_H)
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_include_file("memory.h" HAVE_MEMORY_H)
check_include_file("assert.h" HAVE_ASSERT_H)
check_include_file("arpa/telnet.h" HAVE_ARPA_TELNET_H)
check_include_file("arpa/inet.h" HAVE_ARPA_INET_H)
check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file("sys/socket.h" HAVE_SYS_SOCKET_H)
check_include_file("sys/resource.h" HAVE_SYS_RESOURCE_H)
check_include_file("netinet/in.h" HAVE_NETINET_IN_H)
check_include_file("netdb.h" HAVE_NETDB_H)
check_include_file("signal.h" HAVE_SIGNAL_H)
check_include_file("sys/uio.h" HAVE_SYS_UIO_H)
check_include_file("mcheck.h" HAVE_MCHECK_H)
check_include_file("stdlib.h" HAVE_STDLIB_H)
check_include_file("stdarg.h" HAVE_STDARG_H)
check_include_file("float.h" HAVE_FLOAT_H)

if (HAVE_STDLIB_H AND HAVE_STDARG_H AND HAVE_STRING_H AND HAVE_FLOAT_H)
    set(STDC_HEADERS 1)
endif()

# macros
macro(check_run_return_value CODE EXPECTED_RESULT VAR_NAME)
    set(_file "${CMAKE_BINARY_DIR}/check_run_${VAR_NAME}.c")
    file(WRITE "${_file}" "${CODE}")
    try_run(_run_result _compile_result
            ${CMAKE_BINARY_DIR} ${_file}
    )
    if (_compile_result EQUAL 0 AND _run_result EQUAL ${EXPECTED_RESULT})
        set(${VAR_NAME} TRUE)
    else()
        set(${VAR_NAME} FALSE)
    endif()
endmacro()

# ========== Function checks ==========
foreach(FUNC gettimeofday select snprintf strcasecmp strdup strerror
        stricmp strlcpy strncasecmp strnicmp strstr vsnprintf vprintf
        inet_addr inet_aton)
    string(TOUPPER "${FUNC}" _upper_name)
    check_function_exists(${FUNC} HAVE_${_upper_name})
endforeach()

if (NOT HAVE_VPRINTF)
    check_function_exists(_doprnt HAVE_DOPRNT)
endif()


# ========== Type checks ==========
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
    set(size_t "unsigned")
endif()

if (NOT HAVE_SSIZE_T)
    set(ssize_t int)
endif()

if (NOT HAVE_SOCKLEN_T)
    set(socklen_t int)
endif()

# ========== const ==========
check_c_source_compiles("
int main() {

/* Ultrix mips cc rejects this.  */
typedef int charset[2]; const charset x;
/* SunOS 4.1.1 cc rejects this.  */
char const *const *ccp;
char **p;
/* NEC SVR4.0.2 mips cc rejects this.  */
struct point {int x, y;};
static struct point const zero = {0,0};
/* AIX XL C 1.02.0.0 rejects this.
   It does not let you subtract one const X* pointer from another in an arm
   of an if-expression whose if-part is not a constant expression */
const char *g = \"string\";
ccp = &g + (g ? g-g : 0);
/* HPUX 7.0 cc rejects these. */
++ccp;
p = (char**) ccp;
ccp = (char const *const *) p;
{ /* SCO 3.2v4 cc rejects this.  */
  char *t;
  char const *s = 0 ? (char *) 0 : (char const *) 0;

  *t++ = 0;
}
{ /* Someone thinks the Sun supposedly-ANSI compiler will reject this.  */
  int x[] = {25, 17};
  const int *foo = &x[0];
  ++foo;
}
{ /* Sun SC1.0 ANSI compiler rejects this -- but not the above. */
  typedef const int *iptr;
  iptr p = 0;
  ++p;
}
{ /* AIX XL C 1.02.0.0 rejects this saying
     \"k.c\", line 2.27: 1506-025 (S) Operand must be a modifiable lvalue. */
  struct s { int j; const int *ap[3]; };
  struct s *b; b->j = 5;
}
{ /* ULTRIX-32 V3.1 (Rev 9) vcc rejects this */
  const int foo = 10;
}

; return 0; }
" HAVE_CONST)

if (HAVE_CONST)
    set(CONST_KEYWORD const)
else()
    set(CONST_KEYWORD "")
endif()

# ========== Struct checks ==========
if (HAVE_NETINET_IN_H)
    check_struct_has_member("struct in_addr" s_addr netinet/in.h HAVE_STRUCT_IN_ADDR)
endif()

# ========== MySQL/MariaDB support ==========
find_package(PkgConfig)
if (PkgConfig_FOUND)
    # Try MariaDB first (preferred)
    pkg_check_modules(MARIADB mariadb)
    if (MARIADB_FOUND)
        message(STATUS "Found MariaDB: ${MARIADB_LIBRARIES}")
        list(APPEND EXTRA_LIBS ${MARIADB_LIBRARIES})
        include_directories(${MARIADB_INCLUDE_DIRS})
        set(CIRCLE_MYSQL 1)
    else()
        # Fall back to MySQL
        pkg_check_modules(MYSQL mysqlclient)
        if (MYSQL_FOUND)
            message(STATUS "Found MySQL: ${MYSQL_LIBRARIES}")
            list(APPEND EXTRA_LIBS ${MYSQL_LIBRARIES})
            include_directories(${MYSQL_INCLUDE_DIRS})
            set(CIRCLE_MYSQL 1)
        else()
            # Try manual search for MariaDB/MySQL
            find_path(MYSQL_INCLUDE_DIR mysql.h
                PATHS /usr/include/mariadb /usr/include/mysql /usr/local/include/mariadb /usr/local/include/mysql
                PATH_SUFFIXES mariadb mysql)
            find_library(MYSQL_LIBRARY 
                NAMES mariadb mysqlclient
                PATHS /usr/lib /usr/local/lib
                PATH_SUFFIXES mariadb mysql)
            if (MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY)
                message(STATUS "Found MariaDB/MySQL manually: ${MYSQL_LIBRARY}")
                list(APPEND EXTRA_LIBS ${MYSQL_LIBRARY})
                include_directories(${MYSQL_INCLUDE_DIR})
                set(CIRCLE_MYSQL 1)
            else()
                message(WARNING "MariaDB/MySQL not found. Database support will be disabled.")
            endif()
        endif()
    endif()
endif()

# ========== GD Graphics Library ==========
find_package(PkgConfig)
if (PkgConfig_FOUND)
    pkg_check_modules(GD gdlib)
    if (GD_FOUND)
        message(STATUS "Found GD: ${GD_LIBRARIES}")
        list(APPEND EXTRA_LIBS ${GD_LIBRARIES})
        include_directories(${GD_INCLUDE_DIRS})
    else()
        find_library(GD_LIBRARY gd)
        if (GD_LIBRARY)
            message(STATUS "Found GD manually: ${GD_LIBRARY}")
            list(APPEND EXTRA_LIBS ${GD_LIBRARY})
        endif()
    endif()
endif()

# ========== crypt()/libcrypt ==========

find_library(CRYPT_LIBRARY crypt)
if (CRYPT_LIBRARY)
    message(STATUS "Found libcrypt: ${CRYPT_LIBRARY}")
    list(APPEND EXTRA_LIBS ${CRYPT_LIBRARY})
    set(_saved_lib_list ${CMAKE_REQUIRED_LIBRARIES})
    set(CMAKE_REQUIRED_LIBRARIES ${CRYPT_LIBRARY})
    check_include_file("crypt.h" HAVE_CRYPT_H)
    check_function_exists(crypt CIRCLE_CRYPT)

    check_run_return_value("
#include <string.h>
#include <unistd.h>
${HAVE_CRYPT_H} ? \"#include <crypt.h>\" : \"\"

int main(void)
{
  char pwd[11], pwd2[11];

  strncpy(pwd, (char *)crypt(\"FooBar\", \"BazQux\"), 10);
  pwd[10] = '\\\\0';
  strncpy(pwd2, (char *)crypt(\"xyzzy\", \"BazQux\"), 10);
  pwd2[10] = '\\\\0';
  if (strcmp(pwd, pwd2) == 0)
    exit(0);
  exit(1);
}
    " 0 HAVE_UNSAFE_CRYPT)

    set(CMAKE_REQUIRED_LIBRARIES ${_saved_lib_list})
endif()


# ========== network libs ==========
check_function_exists(gethostbyaddr HAVE_GETHOSTBYADDR)
if (NOT HAVE_GETHOSTBYADDR)
    message(STATUS "gethostbyaddr() not available, trying nsllib")
    find_library(NSL_LIBRARY nsl)
    if (NSL_LIBRARY)
        message(STATUS "...nsllib found.")
        list(APPEND EXTRA_LIBS ${NSL_LIBRARY})
    endif()
endif()

check_function_exists(socket HAVE_SOCKET)
if (NOT HAVE_SOCKET)
    message(STATUS "socket() not available, trying socketlib")
    find_library(SOCKET_LIBRARY socket)
    if (SOCKET_LIBRARY)
        message(STATUS "...socketlib found")
        list(APPEND EXTRA_LIBS ${SOCKET_LIBRARY})
    endif()
endif()

# ========== time.h needs special treatment ==========
check_include_file("sys/time.h" HAVE_SYS_TIME_H)
check_include_file("sys/time.h" HAVE_TIME_H)

if (HAVE_SYS_TIME_H AND HAVE_TIME_H)
    check_c_source_compiles("
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
int main() {
struct tm *tp;
; return 0; }
    " TIME_WITH_SYS_TIME)
endif()

# ========== Determine return value of signal() ==========
check_c_source_compiles("
    #include <signal.h>
    int handler(int sig) { return 0; }
    int main() {
        signal(SIGINT, handler);
        return 1;
    }
" SIGNAL_RETURNS_INT FAIL_REGEX ".*incompatible pointer type.*")

check_c_source_compiles("
    #include <signal.h>
    void handler(int sig) { }
    int main() {
        signal(SIGINT, handler);
        return 1;
    }
" SIGNAL_RETURNS_VOID FAIL_REGEX ".*incompatible pointer type.*")

if (SIGNAL_RETURNS_INT)
    message(STATUS "signal() returns int.")
    set(RETSIGTYPE int)
elseif (SIGNAL_RETURNS_VOID)
    message(STATUS "signal() returns void.")
    set(RETSIGTYPE void)
else()
    message(FATAL_ERROR "Could not determine return value from signal handler.")
endif()

# ========== Define general UNIX-system ==========
if (UNIX)
    set(CIRCLE_UNIX 1)
endif()

set(PROTO_FUNCTIONS
        accept
        bind
        gettimeofday
        atoi
        atol
        bzero
        chdir
        close
        fclose
        fcntl
        fflush
        fprintf
        fputc
        fread
        fscanf
        fseek
        fwrite
        getpeername
        getpid
        getrlimit
        getsockname
        htonl
        htons
        inet_addr
        inet_aton
        inet_ntoa
        listen
        ntohl
        perror
        printf
        qsort
        read
        remove
        rewind
        select
        setitimer
        setrlimit
        setsockopt
        snprintf
        sprintf
        sscanf
        strcasecmp
        strdup
        strerror
        stricmp
        strlcpy
        strncasecmp
        strnicmp
        system
        time
        unlink
        vsnprintf
        write
        socket
)

# Removed conf.h generation - using target compile definitions instead

set(PROTO_DEFINITIONS "")
macro(check_function_prototype FUNCTION)
    set(_code "
#define NO_LIBRARY_PROTOTYPES
#define __COMM_C__
#define __ACT_OTHER_C__
/* Skip including conf.h for prototype checks */
#include \"${CMAKE_SOURCE_DIR}/src/sysdep.h\"
#ifdef ${FUNCTION}
  error - already defined!
#endif
void ${FUNCTION}(int a, char b, int c, char d, int e, char f, int g, char h);

int main() {

; return 0; }
    ")
    string(TOUPPER "${FUNCTION}" _upper_name)
    check_c_source_compiles("${_code}" NEED_${_upper_name}_PROTO FAIL_REGEX ".*incompatible pointer type.*")
    if (NEED_${_upper_name}_PROTO)
        message(STATUS "${FUNCTION}() has no prototype, NEED_${_upper_name}_PROTO set!")
        list(APPEND PROTO_DEFINITIONS "NEED_${_upper_name}_PROTO")
    else()
        message(STATUS "${FUNCTION}() has a prototype, not setting NEED_${_upper_name}_PROTO")
    endif()
endmacro()


foreach (FUNC ${PROTO_FUNCTIONS})
    check_function_prototype(${FUNC})
endforeach()


# ========== Generate conf.h ==========
# Using target compile definitions instead of conf.h



# ========== Source files ==========
# Main C source files
set(SRC_C_FILES
    src/account.c
    src/act.comm.c
    src/act.comm.do_spec_comm.c
    src/act.informative.c
    src/act.item.c
    src/act.offensive.c
    src/act.other.c
    src/act.social.c
    src/act.wizard.c
    src/actionqueues.c
    src/actions.c
    src/aedit.c
    src/ai_cache.c
    src/ai_events.c
    src/ai_security.c
    src/ai_service.c
    src/alchemy.c
    src/asciimap.c
    src/assign_wpn_armor.c
    src/backgrounds.c
    src/ban.c
    src/bardic_performance.c
    src/boards.c
    src/bsd-snprintf.c
    src/cedit.c
    src/char_descs.c
    src/clan.c
    src/clan_economy.c
    src/clan_edit.c
    src/clan_transactions.c
    src/class.c
    src/combat_modes.c
    src/comm.c
    src/config.c
    src/constants.c
    src/craft.c
    src/crafting_new.c
    src/crafting_recipes.c
    src/crafts.c
    src/db.c
    src/deities.c
    src/desc_engine.c
    src/dg_comm.c
    src/dg_db_scripts.c
    src/dg_event.c
    src/dg_handler.c
    src/dg_misc.c
    src/dg_mobcmd.c
    src/dg_objcmd.c
    src/dg_olc.c
    src/dg_scripts.c
    src/dg_triggers.c
    src/dg_variables.c
    src/dg_wldcmd.c
    src/domain_powers.c
    src/domains_schools.c
    src/dotenv.c
    src/encounters.c
    src/evolutions.c
    src/feats.c
    src/fight.c
    src/gain.c
    src/genmob.c
    src/genobj.c
    src/genolc.c
    src/genqst.c
    src/genshp.c
    src/genwld.c
    src/genzon.c
    src/graph.c
    src/grapple.c
    src/handler.c
    src/hedit.c
    src/help.c
    src/helpers.c
    src/hlqedit.c
    src/hlquest.c
    src/house.c
    src/hsedit.c
    src/hunts.c
    src/ibt.c
    src/improved-edit.c
    src/interpreter.c
    src/kdtree.c
    src/limits.c
    src/lists.c
    src/magic.c
    src/mail.c
    src/medit.c
    src/missions.c
    src/mobact.c
    src/modify.c
    src/msgedit.c
    src/mud_event.c
    src/mysql.c
    src/new_mail.c
    src/oasis.c
    src/oasis_copy.c
    src/oasis_delete.c
    src/oasis_list.c
    src/objsave.c
    src/oedit.c
    src/perfmon.c
    src/perlin.c
    src/players.c
    src/prefedit.c
    src/premadebuilds.c
    src/protocol.c
    src/psionics.c
    src/qedit.c
    src/quest.c
    src/race.c
    src/random.c
    src/random_names.c
    src/rank.c
    src/redit.c
    src/roleplay.c
    src/routing.c
    src/sedit.c
    src/shop.c
    src/spec_abilities.c
    src/spec_assign.c
    src/spec_procs.c
    src/specs.artifacts.c
    src/spell_parser.c
    src/spell_prep.c
    src/spellbook_scroll.c
    src/spells.c
    src/staff_events.c
    src/study.c
    src/tedit.c
    src/templates.c
    src/trade.c
    src/transport.c
    src/traps.c
    src/treasure.c
    src/treasure_const.c
    src/utils.c
    src/vessels.c
    src/weather.c
    src/wilderness.c
    src/zedit.c
    src/zmalloc.c
    src/zone_procs.c
)

set(SRC_FILES ${SRC_C_FILES})

# ========== Build main executable FIRST ==========
add_executable(circle ${SRC_FILES})

# Add compile definitions based on detected features
target_compile_definitions(circle PRIVATE
    $<$<BOOL:${CIRCLE_MYSQL}>:CIRCLE_MYSQL>
    $<$<BOOL:${CIRCLE_UNIX}>:CIRCLE_UNIX>
    $<$<BOOL:${CIRCLE_CRYPT}>:CIRCLE_CRYPT>
    $<$<BOOL:${HAVE_FCNTL_H}>:HAVE_FCNTL_H>
    $<$<BOOL:${HAVE_ERRNO_H}>:HAVE_ERRNO_H>
    $<$<BOOL:${HAVE_STRING_H}>:HAVE_STRING_H>
    $<$<BOOL:${HAVE_STRINGS_H}>:HAVE_STRINGS_H>
    $<$<BOOL:${HAVE_LIMITS_H}>:HAVE_LIMITS_H>
    $<$<BOOL:${HAVE_SYS_SELECT_H}>:HAVE_SYS_SELECT_H>
    $<$<BOOL:${HAVE_SYS_WAIT_H}>:HAVE_SYS_WAIT_H>
    $<$<BOOL:${HAVE_SYS_TYPES_H}>:HAVE_SYS_TYPES_H>
    $<$<BOOL:${HAVE_UNISTD_H}>:HAVE_UNISTD_H>
    $<$<BOOL:${HAVE_MEMORY_H}>:HAVE_MEMORY_H>
    $<$<BOOL:${HAVE_ASSERT_H}>:HAVE_ASSERT_H>
    $<$<BOOL:${HAVE_ARPA_TELNET_H}>:HAVE_ARPA_TELNET_H>
    $<$<BOOL:${HAVE_ARPA_INET_H}>:HAVE_ARPA_INET_H>
    $<$<BOOL:${HAVE_SYS_STAT_H}>:HAVE_SYS_STAT_H>
    $<$<BOOL:${HAVE_SYS_SOCKET_H}>:HAVE_SYS_SOCKET_H>
    $<$<BOOL:${HAVE_SYS_RESOURCE_H}>:HAVE_SYS_RESOURCE_H>
    $<$<BOOL:${HAVE_NETINET_IN_H}>:HAVE_NETINET_IN_H>
    $<$<BOOL:${HAVE_NETDB_H}>:HAVE_NETDB_H>
    $<$<BOOL:${HAVE_SIGNAL_H}>:HAVE_SIGNAL_H>
    $<$<BOOL:${HAVE_SYS_UIO_H}>:HAVE_SYS_UIO_H>
    $<$<BOOL:${HAVE_MCHECK_H}>:HAVE_MCHECK_H>
    $<$<BOOL:${HAVE_STDLIB_H}>:HAVE_STDLIB_H>
    $<$<BOOL:${HAVE_STDARG_H}>:HAVE_STDARG_H>
    $<$<BOOL:${HAVE_FLOAT_H}>:HAVE_FLOAT_H>
    $<$<BOOL:${HAVE_SYS_TIME_H}>:HAVE_SYS_TIME_H>
    $<$<BOOL:${HAVE_TIME_H}>:HAVE_TIME_H>
    $<$<BOOL:${HAVE_CRYPT_H}>:HAVE_CRYPT_H>
    $<$<BOOL:${STDC_HEADERS}>:STDC_HEADERS>
    $<$<BOOL:${TIME_WITH_SYS_TIME}>:TIME_WITH_SYS_TIME>
    $<$<BOOL:${HAVE_GETTIMEOFDAY}>:HAVE_GETTIMEOFDAY>
    $<$<BOOL:${HAVE_SELECT}>:HAVE_SELECT>
    $<$<BOOL:${HAVE_SNPRINTF}>:HAVE_SNPRINTF>
    $<$<BOOL:${HAVE_STRCASECMP}>:HAVE_STRCASECMP>
    $<$<BOOL:${HAVE_STRDUP}>:HAVE_STRDUP>
    $<$<BOOL:${HAVE_STRERROR}>:HAVE_STRERROR>
    $<$<BOOL:${HAVE_STRICMP}>:HAVE_STRICMP>
    $<$<BOOL:${HAVE_STRLCPY}>:HAVE_STRLCPY>
    $<$<BOOL:${HAVE_STRNCASECMP}>:HAVE_STRNCASECMP>
    $<$<BOOL:${HAVE_STRNICMP}>:HAVE_STRNICMP>
    $<$<BOOL:${HAVE_STRSTR}>:HAVE_STRSTR>
    $<$<BOOL:${HAVE_VSNPRINTF}>:HAVE_VSNPRINTF>
    $<$<BOOL:${HAVE_VPRINTF}>:HAVE_VPRINTF>
    $<$<BOOL:${HAVE_INET_ADDR}>:HAVE_INET_ADDR>
    $<$<BOOL:${HAVE_INET_ATON}>:HAVE_INET_ATON>
    $<$<BOOL:${HAVE_STRUCT_IN_ADDR}>:HAVE_STRUCT_IN_ADDR>
    $<$<BOOL:${HAVE_CONST}>:HAVE_CONST>
    $<$<BOOL:${HAVE_UNSAFE_CRYPT}>:HAVE_UNSAFE_CRYPT>
    RETSIGTYPE=${RETSIGTYPE}
)

# Add type definitions if needed
if (NOT HAVE_PID_T)
    target_compile_definitions(circle PRIVATE pid_t=int)
endif()
if (NOT HAVE_SIZE_T)
    target_compile_definitions(circle PRIVATE size_t=unsigned)
endif()
if (NOT HAVE_SSIZE_T)
    target_compile_definitions(circle PRIVATE ssize_t=int)
endif()
if (NOT HAVE_SOCKLEN_T)
    target_compile_definitions(circle PRIVATE socklen_t=int)
endif()

# Add function prototype definitions
if (PROTO_DEFINITIONS)
    target_compile_definitions(circle PRIVATE ${PROTO_DEFINITIONS})
endif()

# Link required libraries
target_link_libraries(circle ${EXTRA_LIBS} m pthread)

# Find and link additional libraries
find_library(CURL_LIBRARY curl)
if (CURL_LIBRARY)
    target_link_libraries(circle ${CURL_LIBRARY})
endif()

find_library(SSL_LIBRARY ssl)
if (SSL_LIBRARY)
    target_link_libraries(circle ${SSL_LIBRARY})
endif()

find_library(CRYPTO_LIBRARY crypto)
if (CRYPTO_LIBRARY)
    target_link_libraries(circle ${CRYPTO_LIBRARY})
endif()

# ========== Compiler warnings and options ==========
if (CMAKE_COMPILER_IS_GNUCC)
    # Use C90 but allow C++ style comments for pragmatic reasons
    target_compile_options(circle PRIVATE -std=gnu90)
    
    # Additional warnings for development
    if (DEVELOPER_MODE)
        target_compile_options(circle PRIVATE -Wextra -Wshadow -Wcast-qual 
                               -Wwrite-strings -Wconversion -Wunreachable-code)
    endif()
endif()

# ========== Feature toggles ==========
if (MEMORY_DEBUG)
    message(STATUS "MEMORY_DEBUG is activated")
    target_compile_definitions(circle PRIVATE MEMORY_DEBUG)
endif()

if (DMALLOC)
    message(STATUS "DMALLOC is activated")
    target_compile_definitions(circle PRIVATE DMALLOC)
    find_library(DMALLOC_LIBRARY dmalloc)
    if (DMALLOC_LIBRARY)
        target_link_libraries(circle ${DMALLOC_LIBRARY})
    endif()
endif()

# ========== Build utilities AFTER main executable ==========
# Build utilities by default (can be disabled with -DBUILD_UTILS=OFF)
option(BUILD_UTILS "Build utility programs" ON)

if (BUILD_UTILS)
    # Utilities from util/ directory
    add_executable(autowiz util/autowiz.c)
    add_executable(asciipasswd util/asciipasswd.c)
    add_executable(plrtoascii util/plrtoascii.c)
    add_executable(rebuildAsciiIndex util/rebuildAsciiIndex.c)
    add_executable(rebuildMailIndex util/rebuildMailIndex.c)
    add_executable(shopconv util/shopconv.c)
    add_executable(sign util/sign.c)
    add_executable(split util/split.c)
    add_executable(wld2html util/wld2html.c)
    add_executable(webster util/webster.c)
    
    # hl_events utility has both .c and .h files
    add_executable(hl_events util/hl_events.c)
    
    # Set output directory for utilities
    set_target_properties(autowiz asciipasswd plrtoascii rebuildAsciiIndex
                          rebuildMailIndex shopconv sign split wld2html
                          webster hl_events
                          PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})
    
    # Link libraries for utilities that need them
    target_link_libraries(asciipasswd ${CRYPT_LIBRARY})
endif()

# ========== Build unit tests AFTER main executable ==========
# Build tests by default (can be disabled with -DBUILD_TESTS=OFF)
option(BUILD_TESTS "Build unit tests" ON)

if (BUILD_TESTS)
    # CuTest framework tests
    set(CUTEST_FILES
        unittests/CuTest/CuTest.c
        unittests/CuTest/test.helpers.c
        unittests/CuTest/test.interpreter.c
        unittests/CuTest/test_alignment.c
        unittests/CuTest/test_bounds_checking.c
        unittests/CuTest/test_char_cache.c
        unittests/CuTest/test_char_skills.c
        unittests/CuTest/test_random.c
        unittests/CuTest/test_treasure.c
        unittests/CuTest/test_utils.c
    )
    
    add_executable(cutest ${CUTEST_FILES})
    target_include_directories(cutest PRIVATE src unittests/CuTest)
    set_target_properties(cutest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})
    
    # Individual test executables from unittests/ root
    # Note: These tests have hardcoded relative paths and may need adjustment
    # Commenting out for now as they need proper refactoring
    # add_executable(test_act_comm unittests/test_act.comm.c unittests/mocks.c)
    # add_executable(test_clan unittests/test_clan.c unittests/mocks.c)
    # add_executable(test_staff_events unittests/test_staff_events.c unittests/mocks.c)
    
    # Include directories for individual tests
    # target_include_directories(test_act_comm PRIVATE src unittests)
    # target_include_directories(test_clan PRIVATE src unittests)
    # target_include_directories(test_staff_events PRIVATE src unittests)
    
    # Output directory for test executables
    # set_target_properties(test_act_comm test_clan test_staff_events
    #                       PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})
endif()

# ========== Installation ==========
install(TARGETS circle DESTINATION ${BIN_OUTPUT_DIR})
if (BUILD_UTILS)
    install(TARGETS autowiz asciipasswd plrtoascii rebuildAsciiIndex
                    rebuildMailIndex shopconv sign split wld2html
                    webster hl_events
            DESTINATION ${BIN_OUTPUT_DIR})
endif()
if (BUILD_TESTS)
    install(TARGETS cutest
            DESTINATION ${BIN_OUTPUT_DIR})
endif()

# ========== Summary ==========
message(STATUS "========================================")
message(STATUS "LuminariMUD CMake Configuration Summary:")
message(STATUS "  C Standard: GNU C90 (C89 with GNU extensions)")
message(STATUS "  MySQL Support: ${CIRCLE_MYSQL}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Utilities: ${BUILD_UTILS}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Binary Output: ${BIN_OUTPUT_DIR}")
message(STATUS "========================================")
