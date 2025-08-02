/* LuminariMUD Unit Tests for Clan System */

#include "CuTest/CuTest.h"
#include "../clan.h"
#include "../structs.h"
#include "../utils.h"
#include "../db.h"
#include "../comm.h"

#include <string.h>
#include <stdlib.h>

/* Mock global variables */
struct clan_data *clan_list = NULL;
struct claim_data *claim_list = NULL;
int num_of_clans = 0;
struct clan_hash_entry *clan_hash_table[CLAN_HASH_SIZE];
struct player_index_element *player_table = NULL;
int top_of_p_table = 0;

/* Test helper functions */
static struct clan_data *create_test_clan(clan_vnum vnum, const char *name) {
    struct clan_data *clan;
    CREATE(clan, struct clan_data, 1);
    clear_clan_vals(clan);
    
    clan->vnum = vnum;
    clan->clan_name = strdup(name);
    clan->description = strdup("Test clan description");
    clan->abrev = strdup("TEST");
    clan->leader = 1000; /* Test player ID */
    clan->ranks = DEFAULT_CLAN_RANKS;
    clan->applev = 1;
    clan->appfee = 1000;
    clan->taxrate = 10;
    clan->treasure = 50000;
    clan->max_members = DEFAULT_MAX_MEMBERS;
    
    /* Set default rank names */
    clan->rank_name[0] = strdup("Duke");
    clan->rank_name[1] = strdup("Count");
    clan->rank_name[2] = strdup("Baron");
    clan->rank_name[3] = strdup("Lord");
    clan->rank_name[4] = strdup("Member");
    clan->rank_name[5] = strdup("Recruit");
    
    return clan;
}

static void cleanup_test_clan(struct clan_data *clan) {
    if (!clan) return;
    free_clan(clan);
}

static struct char_data *create_test_character(long id, const char *name, int level) {
    struct char_data *ch;
    CREATE(ch, struct char_data, 1);
    
    ch->player.name = strdup(name);
    ch->player.level = level;
    ch->char_specials.saved.idnum = id;
    ch->player_specials = calloc(1, sizeof(struct player_special_data));
    ch->player_specials->saved.clan = NO_CLAN;
    ch->player_specials->saved.clanrank = NO_CLANRANK;
    
    return ch;
}

static void cleanup_test_character(struct char_data *ch) {
    if (!ch) return;
    if (ch->player.name) free(ch->player.name);
    if (ch->player_specials) free(ch->player_specials);
    free(ch);
}

/* Test clear_clan_vals function */
void Test_clear_clan_vals(CuTest *tc) {
    struct clan_data clan;
    
    /* Initialize with garbage values */
    memset(&clan, 0xFF, sizeof(clan));
    
    clear_clan_vals(&clan);
    
    /* Check all values are properly cleared */
    /* Note: clear_clan_vals doesn't clear vnum, leader, ranks, rank_name[], or privilege[] */
    CuAssertPtrEquals(tc, NULL, clan.clan_name);
    CuAssertPtrEquals(tc, NULL, clan.description);
    CuAssertIntEquals(tc, 0, clan.applev);
    CuAssertIntEquals(tc, 0, clan.appfee);
    CuAssertIntEquals(tc, 0, clan.taxrate);
    CuAssertIntEquals(tc, 0, clan.war_timer);
    CuAssertIntEquals(tc, 0, clan.hall);
    CuAssertIntEquals(tc, 0, clan.treasure);
    CuAssertIntEquals(tc, 0, clan.pk_win);
    CuAssertIntEquals(tc, 0, clan.pk_lose);
    CuAssertIntEquals(tc, 0, clan.raided);
    CuAssertPtrEquals(tc, NULL, clan.abrev);
    CuAssertIntEquals(tc, FALSE, clan.modified);
    CuAssertIntEquals(tc, DEFAULT_MAX_MEMBERS, clan.max_members);
    
    /* Check cache values */
    CuAssertIntEquals(tc, 0, clan.cached_member_count);
    CuAssertIntEquals(tc, 0, clan.cached_member_power);
    CuAssertIntEquals(tc, 0, clan.cache_timestamp);
    
    /* Check statistics */
    CuAssertIntEquals(tc, 0, clan.total_deposits);
    CuAssertIntEquals(tc, 0, clan.total_withdrawals);
    CuAssertIntEquals(tc, 0, clan.total_members_joined);
    CuAssertIntEquals(tc, 0, clan.total_members_left);
    CuAssertIntEquals(tc, 0, clan.total_zones_claimed);
    CuAssertIntEquals(tc, 0, clan.current_zones_owned);
    CuAssertIntEquals(tc, 0, clan.highest_member_count);
    CuAssertIntEquals(tc, 0, clan.total_taxes_collected);
    CuAssertIntEquals(tc, 0, clan.total_wars_won);
    CuAssertIntEquals(tc, 0, clan.total_wars_lost);
    CuAssertIntEquals(tc, 0, clan.total_alliances_formed);
    
    /* Check arrays are cleared */
    for (int i = 0; i < MAX_CLANS; i++) {
        CuAssertIntEquals(tc, FALSE, clan.at_war[i]);
        CuAssertIntEquals(tc, FALSE, clan.allies[i]);
    }
}

/* Test real_clan function */
void Test_real_clan(CuTest *tc) {
    struct clan_data *test_clan;
    clan_rnum result;
    
    /* Setup test data */
    num_of_clans = 3;
    CREATE(clan_list, struct clan_data, num_of_clans);
    
    clan_list[0].vnum = 1;
    clan_list[1].vnum = 5;
    clan_list[2].vnum = 10;
    
    /* Test finding existing clans */
    result = real_clan(1);
    CuAssertIntEquals(tc, 0, result);
    
    result = real_clan(5);
    CuAssertIntEquals(tc, 1, result);
    
    result = real_clan(10);
    CuAssertIntEquals(tc, 2, result);
    
    /* Test non-existent clan */
    result = real_clan(999);
    CuAssertIntEquals(tc, NO_CLAN, result);
    
    /* Test NO_CLAN input */
    result = real_clan(NO_CLAN);
    CuAssertIntEquals(tc, NO_CLAN, result);
    
    /* Cleanup */
    free(clan_list);
    clan_list = NULL;
    num_of_clans = 0;
}

/* Test add_clan function */
void Test_add_clan(CuTest *tc) {
    struct clan_data *new_clan;
    bool result;
    
    /* Initialize */
    init_clan_hash();
    num_of_clans = 0;
    clan_list = NULL;
    
    /* Create and add first clan */
    new_clan = create_test_clan(1, "Test Clan 1");
    result = add_clan(new_clan);
    CuAssertTrue(tc, result);
    CuAssertIntEquals(tc, 1, num_of_clans);
    CuAssertStrEquals(tc, "Test Clan 1", clan_list[0].clan_name);
    
    /* Add second clan */
    new_clan = create_test_clan(2, "Test Clan 2");
    result = add_clan(new_clan);
    CuAssertTrue(tc, result);
    CuAssertIntEquals(tc, 2, num_of_clans);
    
    /* Try to add duplicate vnum */
    new_clan = create_test_clan(1, "Duplicate");
    result = add_clan(new_clan);
    CuAssertTrue(tc, !result);
    CuAssertIntEquals(tc, 2, num_of_clans);
    cleanup_test_clan(new_clan);
    
    /* Cleanup */
    free_clan_list();
    free_clan_hash();
}

/* Test remove_clan function */
void Test_remove_clan(CuTest *tc) {
    struct clan_data *clan1, *clan2, *clan3;
    bool result;
    
    /* Initialize and add test clans */
    init_clan_hash();
    num_of_clans = 0;
    clan_list = NULL;
    
    clan1 = create_test_clan(1, "Clan 1");
    clan2 = create_test_clan(2, "Clan 2");
    clan3 = create_test_clan(3, "Clan 3");
    
    add_clan(clan1);
    add_clan(clan2);
    add_clan(clan3);
    
    CuAssertIntEquals(tc, 3, num_of_clans);
    
    /* Remove middle clan */
    result = remove_clan(2);
    CuAssertTrue(tc, result);
    CuAssertIntEquals(tc, 2, num_of_clans);
    CuAssertIntEquals(tc, 1, clan_list[0].vnum);
    CuAssertIntEquals(tc, 3, clan_list[1].vnum);
    
    /* Try to remove non-existent clan */
    result = remove_clan(999);
    CuAssertTrue(tc, !result);
    CuAssertIntEquals(tc, 2, num_of_clans);
    
    /* Remove remaining clans */
    result = remove_clan(1);
    CuAssertTrue(tc, result);
    CuAssertIntEquals(tc, 1, num_of_clans);
    
    result = remove_clan(3);
    CuAssertTrue(tc, result);
    CuAssertIntEquals(tc, 0, num_of_clans);
    CuAssertPtrEquals(tc, NULL, clan_list);
    
    /* Cleanup */
    free_clan_hash();
}

/* Test check_clan_leader function */
void Test_check_clan_leader(CuTest *tc) {
    struct char_data *ch_leader, *ch_member, *ch_noclan;
    struct clan_data *clan;
    
    /* Setup */
    init_clan_hash();
    num_of_clans = 0;
    clan_list = NULL;
    
    clan = create_test_clan(1, "Test Clan");
    clan->leader = 1000;
    add_clan(clan);
    
    /* Create test characters */
    ch_leader = create_test_character(1000, "Leader", 50);
    ch_leader->player_specials->saved.clan = 1;
    ch_leader->player_specials->saved.clanrank = 1;
    
    ch_member = create_test_character(1001, "Member", 30);
    ch_member->player_specials->saved.clan = 1;
    ch_member->player_specials->saved.clanrank = 5;
    
    ch_noclan = create_test_character(1002, "NoClan", 20);
    
    /* Test leader check */
    CuAssertTrue(tc, check_clan_leader(ch_leader));
    CuAssertTrue(tc, !check_clan_leader(ch_member));
    CuAssertTrue(tc, !check_clan_leader(ch_noclan));
    
    /* Test NULL character */
    CuAssertTrue(tc, !check_clan_leader(NULL));
    
    /* Cleanup */
    cleanup_test_character(ch_leader);
    cleanup_test_character(ch_member);
    cleanup_test_character(ch_noclan);
    free_clan_list();
    free_clan_hash();
}

/* Test check_clanpriv function */
void Test_check_clanpriv(CuTest *tc) {
    struct char_data *ch_leader, *ch_officer, *ch_member;
    struct clan_data *clan;
    
    /* Setup */
    init_clan_hash();
    num_of_clans = 0;
    clan_list = NULL;
    
    clan = create_test_clan(1, "Test Clan");
    
    /* Set up privileges - CP_WITHDRAW requires rank 3 or better */
    clan->privilege[CP_WITHDRAW] = 3;
    clan->privilege[CP_OWNER] = 0; /* Leader only */
    clan->privilege[CP_WHERE] = 6; /* All members */
    
    add_clan(clan);
    
    /* Create test characters */
    ch_leader = create_test_character(1000, "Leader", 50);
    ch_leader->player_specials->saved.clan = 1;
    ch_leader->player_specials->saved.clanrank = 1;
    
    ch_officer = create_test_character(1001, "Officer", 40);
    ch_officer->player_specials->saved.clan = 1;
    ch_officer->player_specials->saved.clanrank = 3;
    
    ch_member = create_test_character(1002, "Member", 20);
    ch_member->player_specials->saved.clan = 1;
    ch_member->player_specials->saved.clanrank = 5;
    
    /* Test leader has all privileges */
    CuAssertTrue(tc, check_clanpriv(ch_leader, CP_WITHDRAW));
    CuAssertTrue(tc, check_clanpriv(ch_leader, CP_OWNER));
    CuAssertTrue(tc, check_clanpriv(ch_leader, CP_WHERE));
    
    /* Test officer privileges */
    CuAssertTrue(tc, check_clanpriv(ch_officer, CP_WITHDRAW));
    CuAssertTrue(tc, !check_clanpriv(ch_officer, CP_OWNER));
    CuAssertTrue(tc, check_clanpriv(ch_officer, CP_WHERE));
    
    /* Test member privileges */
    CuAssertTrue(tc, !check_clanpriv(ch_member, CP_WITHDRAW));
    CuAssertTrue(tc, !check_clanpriv(ch_member, CP_OWNER));
    CuAssertTrue(tc, check_clanpriv(ch_member, CP_WHERE));
    
    /* Test NULL character */
    CuAssertTrue(tc, !check_clanpriv(NULL, CP_WHERE));
    
    /* Cleanup */
    cleanup_test_character(ch_leader);
    cleanup_test_character(ch_officer);
    cleanup_test_character(ch_member);
    free_clan_list();
    free_clan_hash();
}

/* Test get_clan_by_name function */
void Test_get_clan_by_name(CuTest *tc) {
    struct clan_data *clan1, *clan2;
    clan_rnum result;
    
    /* Setup */
    init_clan_hash();
    num_of_clans = 0;
    clan_list = NULL;
    
    clan1 = create_test_clan(1, "Knights of Valor");
    clan2 = create_test_clan(2, "Shadow Guild");
    
    add_clan(clan1);
    add_clan(clan2);
    
    /* Test exact match */
    result = get_clan_by_name("Knights of Valor");
    CuAssertIntEquals(tc, 0, result);
    
    result = get_clan_by_name("Shadow Guild");
    CuAssertIntEquals(tc, 1, result);
    
    /* Test partial match */
    result = get_clan_by_name("knight");
    CuAssertIntEquals(tc, 0, result);
    
    result = get_clan_by_name("shad");
    CuAssertIntEquals(tc, 1, result);
    
    /* Test case insensitive */
    result = get_clan_by_name("KNIGHTS");
    CuAssertIntEquals(tc, 0, result);
    
    /* Test non-existent clan */
    result = get_clan_by_name("Nonexistent");
    CuAssertIntEquals(tc, NO_CLAN, result);
    
    /* Test NULL/empty input */
    result = get_clan_by_name(NULL);
    CuAssertIntEquals(tc, NO_CLAN, result);
    
    result = get_clan_by_name("");
    CuAssertIntEquals(tc, NO_CLAN, result);
    
    /* Cleanup */
    free_clan_list();
    free_clan_hash();
}

/* Test are_clans_allied function */
void Test_are_clans_allied(CuTest *tc) {
    struct clan_data *clan1, *clan2, *clan3;
    
    /* Setup */
    init_clan_hash();
    num_of_clans = 0;
    clan_list = NULL;
    
    clan1 = create_test_clan(1, "Clan 1");
    clan2 = create_test_clan(2, "Clan 2");
    clan3 = create_test_clan(3, "Clan 3");
    
    add_clan(clan1);
    add_clan(clan2);
    add_clan(clan3);
    
    /* Set up alliances */
    clan_list[0].allies[1] = TRUE; /* Clan 1 allied with Clan 2 */
    clan_list[1].allies[0] = TRUE; /* Clan 2 allied with Clan 1 */
    
    /* Test allied clans */
    CuAssertTrue(tc, are_clans_allied(0, 1));
    CuAssertTrue(tc, are_clans_allied(1, 0));
    
    /* Test non-allied clans */
    CuAssertTrue(tc, !are_clans_allied(0, 2));
    CuAssertTrue(tc, !are_clans_allied(1, 2));
    
    /* Test same clan */
    CuAssertTrue(tc, !are_clans_allied(0, 0));
    
    /* Test invalid clan indices */
    CuAssertTrue(tc, !are_clans_allied(-1, 0));
    CuAssertTrue(tc, !are_clans_allied(0, num_of_clans));
    CuAssertTrue(tc, !are_clans_allied(num_of_clans, num_of_clans));
    
    /* Cleanup */
    free_clan_list();
    free_clan_hash();
}

/* Test are_clans_at_war function */
void Test_are_clans_at_war(CuTest *tc) {
    struct clan_data *clan1, *clan2, *clan3;
    
    /* Setup */
    init_clan_hash();
    num_of_clans = 0;
    clan_list = NULL;
    
    clan1 = create_test_clan(1, "Clan 1");
    clan2 = create_test_clan(2, "Clan 2");
    clan3 = create_test_clan(3, "Clan 3");
    
    add_clan(clan1);
    add_clan(clan2);
    add_clan(clan3);
    
    /* Set up wars */
    clan_list[0].at_war[2] = TRUE; /* Clan 1 at war with Clan 3 */
    clan_list[2].at_war[0] = TRUE; /* Clan 3 at war with Clan 1 */
    
    /* Test warring clans */
    CuAssertTrue(tc, are_clans_at_war(0, 2));
    CuAssertTrue(tc, are_clans_at_war(2, 0));
    
    /* Test peaceful clans */
    CuAssertTrue(tc, !are_clans_at_war(0, 1));
    CuAssertTrue(tc, !are_clans_at_war(1, 2));
    
    /* Test same clan */
    CuAssertTrue(tc, !are_clans_at_war(0, 0));
    
    /* Test invalid clan indices */
    CuAssertTrue(tc, !are_clans_at_war(-1, 0));
    CuAssertTrue(tc, !are_clans_at_war(0, num_of_clans));
    
    /* Cleanup */
    free_clan_list();
    free_clan_hash();
}

/* Test hash table functions */
void Test_clan_hash_functions(CuTest *tc) {
    clan_rnum result;
    
    /* Initialize */
    init_clan_hash();
    num_of_clans = 0;
    clan_list = NULL;
    
    /* Add clans using add_clan which should update hash */
    struct clan_data *clan1 = create_test_clan(100, "Clan 100");
    struct clan_data *clan2 = create_test_clan(200, "Clan 200");
    
    add_clan(clan1);
    add_clan(clan2);
    
    /* Test hash lookups work via real_clan */
    result = real_clan(100);
    CuAssertIntEquals(tc, 0, result);
    
    result = real_clan(200);
    CuAssertIntEquals(tc, 1, result);
    
    result = real_clan(999);
    CuAssertIntEquals(tc, NO_CLAN, result);
    
    /* Test removing from hash */
    remove_clan(100);
    result = real_clan(100);
    CuAssertIntEquals(tc, NO_CLAN, result);
    
    /* Clan 200 should now be at index 0 */
    result = real_clan(200);
    CuAssertIntEquals(tc, 0, result);
    
    /* Cleanup */
    free_clan_list();
    free_clan_hash();
}

/* Create test suite */
CuSuite *ClanSystemGetSuite(void) {
    CuSuite *suite = CuSuiteNew();
    
    SUITE_ADD_TEST(suite, Test_clear_clan_vals);
    SUITE_ADD_TEST(suite, Test_real_clan);
    SUITE_ADD_TEST(suite, Test_add_clan);
    SUITE_ADD_TEST(suite, Test_remove_clan);
    SUITE_ADD_TEST(suite, Test_check_clan_leader);
    SUITE_ADD_TEST(suite, Test_check_clanpriv);
    SUITE_ADD_TEST(suite, Test_get_clan_by_name);
    SUITE_ADD_TEST(suite, Test_are_clans_allied);
    SUITE_ADD_TEST(suite, Test_are_clans_at_war);
    SUITE_ADD_TEST(suite, Test_clan_hash_functions);
    
    return suite;
}

/* Main test runner */
int main(void) {
    CuString *output = CuStringNew();
    CuSuite *suite = ClanSystemGetSuite();
    
    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
    
    int failCount = suite->failCount;
    CuSuiteDelete(suite);
    CuStringDelete(output);
    
    return failCount;
}