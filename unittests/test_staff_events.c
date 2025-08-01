/* LuminariMUD Unit Tests for Staff Event System */

#include "CuTest/CuTest.h"
#include "../src/staff_events.h"
#include "../src/structs.h"
#include "../src/utils.h"
#include "../src/comm.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>

/* Test Event State Management */
void Test_event_state_management(CuTest *tc) {
    /* Test initial state */
    clear_event_state();
    CuAssertIntEquals(tc, 0, is_event_active());
    CuAssertIntEquals(tc, UNDEFINED_EVENT, get_active_event());
    CuAssertIntEquals(tc, 0, get_event_time_remaining());
    
    /* Test setting event state */
    set_event_state(JACKALOPE_HUNT, 100);
    CuAssertIntEquals(tc, 1, is_event_active());
    CuAssertIntEquals(tc, JACKALOPE_HUNT, get_active_event());
    CuAssertIntEquals(tc, 100, get_event_time_remaining());
    
    /* Test clearing state */
    clear_event_state();
    CuAssertIntEquals(tc, 0, is_event_active());
    CuAssertIntEquals(tc, UNDEFINED_EVENT, get_active_event());
    
    /* Test delay management */
    set_event_delay(50);
    CuAssertIntEquals(tc, 50, get_event_delay());
}

/* Test Start Staff Event Function */
void Test_start_staff_event(CuTest *tc) {
    event_result_t result;
    
    /* Test invalid event numbers */
    result = start_staff_event(-1);
    CuAssertIntEquals(tc, EVENT_ERROR_INVALID_NUM, result);
    
    result = start_staff_event(999);
    CuAssertIntEquals(tc, EVENT_ERROR_INVALID_NUM, result);
    
    /* Test valid event start */
    clear_event_state();
    set_event_delay(0);
    result = start_staff_event(JACKALOPE_HUNT);
    CuAssertIntEquals(tc, EVENT_SUCCESS, result);
    CuAssertIntEquals(tc, 1, is_event_active());
    CuAssertIntEquals(tc, JACKALOPE_HUNT, get_active_event());
    
    /* Test concurrent event prevention */
    result = start_staff_event(THE_PRISONER_EVENT);
    CuAssertTrue(tc, result != EVENT_SUCCESS); /* Should fail */
    
    /* Clean up */
    end_staff_event(JACKALOPE_HUNT);
}

/* Test End Staff Event Function */
void Test_end_staff_event(CuTest *tc) {
    event_result_t result;
    
    /* Test ending non-active event */
    clear_event_state();
    result = end_staff_event(JACKALOPE_HUNT);
    CuAssertIntEquals(tc, EVENT_SUCCESS, result);
    
    /* Test ending invalid event */
    result = end_staff_event(-1);
    CuAssertIntEquals(tc, EVENT_ERROR_INVALID_NUM, result);
    
    result = end_staff_event(999);
    CuAssertIntEquals(tc, EVENT_ERROR_INVALID_NUM, result);
    
    /* Test normal end sequence */
    clear_event_state();
    set_event_delay(0);
    start_staff_event(JACKALOPE_HUNT);
    
    result = end_staff_event(JACKALOPE_HUNT);
    CuAssertIntEquals(tc, EVENT_SUCCESS, result);
    CuAssertIntEquals(tc, 0, is_event_active());
    CuAssertTrue(tc, get_event_delay() > 0); /* Should set cleanup delay */
}

/* Test Coordinate Generation and Caching */
void Test_coordinate_generation(CuTest *tc) {
    int x_coord = 0, y_coord = 0;
    int i = 0;
    
    /* Test multiple coordinate generations */
    for (i = 0; i < 100; i++) {
        get_cached_coordinates(&x_coord, &y_coord);
        
        /* Validate coordinates are within Jackalope boundaries */
        CuAssertTrue(tc, x_coord >= JACKALOPE_WEST_X);
        CuAssertTrue(tc, x_coord <= JACKALOPE_EAST_X);
        CuAssertTrue(tc, y_coord >= JACKALOPE_SOUTH_Y);
        CuAssertTrue(tc, y_coord <= JACKALOPE_NORTH_Y);
    }
}

/* Test Jackalope Mob Counting */
void Test_jackalope_mob_counting(CuTest *tc) {
    int easy_count = 0, med_count = 0, hard_count = 0;
    
    /* Test counting with NULL pointers (should not crash) */
    count_jackalope_mobs(NULL, NULL, NULL);
    
    /* Test counting with valid pointers */
    count_jackalope_mobs(&easy_count, &med_count, &hard_count);
    
    /* Counts should be non-negative */
    CuAssertTrue(tc, easy_count >= 0);
    CuAssertTrue(tc, med_count >= 0);
    CuAssertTrue(tc, hard_count >= 0);
    
    /* Test individual vs optimized counting consistency */
    int individual_easy = mob_ingame_count(EASY_JACKALOPE);
    CuAssertIntEquals(tc, easy_count, individual_easy);
}

/* Test Object Pooling System */
void Test_object_pool(CuTest *tc) {
    struct obj_data *obj1, *obj2;
    
    /* Initialize pool */
    init_object_pool();
    
    /* Test getting objects from pool */
    obj1 = get_pooled_object(JACKALOPE_HIDE);
    CuAssertPtrNotNull(tc, obj1);
    
    obj2 = get_pooled_object(PRISTINE_HORN);
    CuAssertPtrNotNull(tc, obj2);
    
    /* Test returning objects to pool */
    return_object_to_pool(obj1);
    return_object_to_pool(obj2);
    
    /* Test cleanup */
    cleanup_object_pool();
}

/* Test Hash Table System */
void Test_hash_tables(CuTest *tc) {
    int result;
    
    /* Initialize hash tables */
    init_hash_tables();
    
    /* Test event lookups */
    result = hash_lookup_event(EASY_JACKALOPE);
    CuAssertIntEquals(tc, JACKALOPE_HUNT, result);
    
    result = hash_lookup_event(MED_JACKALOPE);
    CuAssertIntEquals(tc, JACKALOPE_HUNT, result);
    
    result = hash_lookup_event(HARD_JACKALOPE);
    CuAssertIntEquals(tc, JACKALOPE_HUNT, result);
    
    /* Test non-existent lookup */
    result = hash_lookup_event(999);
    CuAssertIntEquals(tc, -1, result);
    
    /* Test mobile count caching */
    hash_update_mobile_count(EASY_JACKALOPE, 42);
    result = hash_lookup_mobile_count(EASY_JACKALOPE);
    CuAssertIntEquals(tc, 42, result);
    
    /* Test non-cached lookup */
    result = hash_lookup_mobile_count(999);
    CuAssertIntEquals(tc, -1, result);
    
    /* Cleanup */
    cleanup_hash_tables();
}

/* Test Event Lifecycle Integration */
void Test_event_lifecycle(CuTest *tc) {
    event_result_t result;
    
    /* Phase 1: Clean initial state */
    clear_event_state();
    set_event_delay(0);
    CuAssertIntEquals(tc, 0, is_event_active());
    
    /* Phase 2: Start event */
    result = start_staff_event(JACKALOPE_HUNT);
    CuAssertIntEquals(tc, EVENT_SUCCESS, result);
    CuAssertIntEquals(tc, 1, is_event_active());
    CuAssertIntEquals(tc, JACKALOPE_HUNT, get_active_event());
    
    /* Phase 3: Test operations during active state */
    int easy_count = 0, med_count = 0, hard_count = 0;
    count_jackalope_mobs(&easy_count, &med_count, &hard_count);
    /* Counts should be accessible during event */
    
    /* Phase 4: End event */
    result = end_staff_event(JACKALOPE_HUNT);
    CuAssertIntEquals(tc, EVENT_SUCCESS, result);
    CuAssertIntEquals(tc, 0, is_event_active());
    CuAssertTrue(tc, get_event_delay() > 0);
}

/* Test Error Conditions */
void Test_error_conditions(CuTest *tc) {
    event_result_t result;
    
    /* Test starting event during cleanup delay */
    set_event_delay(10);
    result = start_staff_event(JACKALOPE_HUNT);
    CuAssertTrue(tc, result != EVENT_SUCCESS);
    
    /* Test invalid state recovery */
    set_event_state(999, 100); /* Invalid event */
    CuAssertIntEquals(tc, 999, get_active_event());
    
    clear_event_state();
    CuAssertIntEquals(tc, UNDEFINED_EVENT, get_active_event());
    
    /* Test negative time handling */
    set_event_state(JACKALOPE_HUNT, -10);
    CuAssertIntEquals(tc, -10, get_event_time_remaining());
    clear_event_state();
}

/* Test Performance Operations */
void Test_performance_operations(CuTest *tc) {
    clock_t start, end;
    double cpu_time_used;
    int i;
    
    /* Test coordinate generation performance */
    start = clock();
    for (i = 0; i < 1000; i++) {
        int x = 0, y = 0;
        get_cached_coordinates(&x, &y);
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    
    /* Should be very fast (under 1 second for 1000 operations) */
    CuAssertTrue(tc, cpu_time_used < 1.0);
    
    /* Test hash table performance */
    init_hash_tables();
    
    start = clock();
    for (i = 0; i < 1000; i++) {
        hash_lookup_event(EASY_JACKALOPE);
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    
    /* Hash lookups should be very fast */
    CuAssertTrue(tc, cpu_time_used < 0.1);
    
    cleanup_hash_tables();
}

/* Test Boundary Conditions */
void Test_boundary_conditions(CuTest *tc) {
    /* Test event number boundaries */
    CuAssertIntEquals(tc, EVENT_ERROR_INVALID_NUM, start_staff_event(-1));
    CuAssertIntEquals(tc, EVENT_ERROR_INVALID_NUM, start_staff_event(NUM_STAFF_EVENTS));
    
    /* Test valid boundaries */
    clear_event_state();
    set_event_delay(0);
    CuAssertIntEquals(tc, EVENT_SUCCESS, start_staff_event(0));
    end_staff_event(0);
    
    clear_event_state();
    set_event_delay(0);
    CuAssertIntEquals(tc, EVENT_SUCCESS, start_staff_event(NUM_STAFF_EVENTS - 1));
    end_staff_event(NUM_STAFF_EVENTS - 1);
}

/* Test Constants and Definitions */
void Test_constants_and_definitions(CuTest *tc) {
    /* Test that all required constants are defined */
    CuAssertTrue(tc, NUM_STAFF_EVENTS > 0);
    CuAssertTrue(tc, STAFF_EVENT_FIELDS > 0);
    CuAssertTrue(tc, JACKALOPE_HUNT >= 0);
    CuAssertTrue(tc, THE_PRISONER_EVENT >= 0);
    CuAssertIntEquals(tc, -1, UNDEFINED_EVENT);
    
    /* Test coordinate boundaries are sensible */
    CuAssertTrue(tc, JACKALOPE_WEST_X < JACKALOPE_EAST_X);
    CuAssertTrue(tc, JACKALOPE_SOUTH_Y < JACKALOPE_NORTH_Y);
    
    /* Test mob count limits */
    CuAssertTrue(tc, NUM_JACKALOPE_EACH > 0);
    CuAssertTrue(tc, NUM_JACKALOPE_EACH <= 300); /* Per audit recommendation */
    
    /* Test cache sizes */
    CuAssertTrue(tc, COORD_CACHE_SIZE > 0);
    CuAssertTrue(tc, COORD_BATCH_GENERATION_SIZE > 0);
    CuAssertTrue(tc, OBJECT_POOL_SIZE > 0);
}

/* Create test suite */
CuSuite *StaffEventSystemGetSuite(void) {
    CuSuite *suite = CuSuiteNew();
    
    SUITE_ADD_TEST(suite, Test_event_state_management);
    SUITE_ADD_TEST(suite, Test_start_staff_event);
    SUITE_ADD_TEST(suite, Test_end_staff_event);
    SUITE_ADD_TEST(suite, Test_coordinate_generation);
    SUITE_ADD_TEST(suite, Test_jackalope_mob_counting);
    SUITE_ADD_TEST(suite, Test_object_pool);
    SUITE_ADD_TEST(suite, Test_hash_tables);
    SUITE_ADD_TEST(suite, Test_event_lifecycle);
    SUITE_ADD_TEST(suite, Test_error_conditions);
    SUITE_ADD_TEST(suite, Test_performance_operations);
    SUITE_ADD_TEST(suite, Test_boundary_conditions);
    SUITE_ADD_TEST(suite, Test_constants_and_definitions);
    
    return suite;
}

/* Main test runner */
int main(void) {
    CuString *output = CuStringNew();
    CuSuite *suite = StaffEventSystemGetSuite();
    
    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
    
    int failCount = suite->failCount;
    CuSuiteDelete(suite);
    CuStringDelete(output);
    
    return failCount;
}