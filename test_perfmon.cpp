/**
 * @file test_perfmon.cpp
 * @brief Unit tests for the performance monitoring system
 *
 * This file contains comprehensive unit tests for the perfmon.cpp module
 * to verify that all improvements and fixes work correctly.
 */

#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>

// Mock the external constant for testing
const unsigned PERF_pulse_per_second = 10;

// Include the perfmon implementation
#include "perfmon.cpp"

/**
 * @brief Test fixture class for performance monitoring tests
 */
class PerfMonTest
{
public:
    /**
     * @brief Run all tests
     */
    static void RunAllTests()
    {
        std::cout << "Running Performance Monitor Tests...\n";
        
        TestPerfIntvlDataBasic();
        TestPerfIntvlDataCircularBuffer();
        TestPerfIntvlDataAggregation();
        TestPerfProfMgrMemoryManagement();
        TestPerfProfMgrReportGeneration();
        TestBufferSafety();
        TestInputValidation();
        TestPulseLogging();
        TestThresholdTracking();
        
        std::cout << "All tests passed!\n";
    }

private:
    /**
     * @brief Test basic PerfIntvlData functionality
     */
    static void TestPerfIntvlDataBasic()
    {
        std::cout << "Testing PerfIntvlData basic functionality...\n";
        
        PerfIntvlData data(5, nullptr);
        
        // Test empty state
        assert(data.GetCount() == 0);
        assert(data.GetAvgAvg() == 0.0);
        assert(data.GetMinMin() == INFINITY);
        assert(data.GetMaxMax() == 0.0);
        
        // Add some data
        data.AddData(10.0, 8.0, 12.0);
        assert(data.GetCount() == 1);
        assert(data.GetAvgAvg() == 10.0);
        assert(data.GetMinMin() == 8.0);
        assert(data.GetMaxMax() == 12.0);
        
        // Add more data
        data.AddData(20.0, 15.0, 25.0);
        assert(data.GetCount() == 2);
        assert(data.GetAvgAvg() == 15.0);  // (10 + 20) / 2
        assert(data.GetMinMin() == 8.0);   // min(8, 15)
        assert(data.GetMaxMax() == 25.0);  // max(12, 25)
        
        std::cout << "  ✓ Basic functionality tests passed\n";
    }

    /**
     * @brief Test circular buffer behavior
     */
    static void TestPerfIntvlDataCircularBuffer()
    {
        std::cout << "Testing PerfIntvlData circular buffer...\n";
        
        PerfIntvlData data(3, nullptr);
        
        // Fill the buffer
        data.AddData(10.0, 10.0, 10.0);
        data.AddData(20.0, 20.0, 20.0);
        data.AddData(30.0, 30.0, 30.0);
        assert(data.GetCount() == 3);
        
        // Add one more to trigger wraparound
        data.AddData(40.0, 40.0, 40.0);
        assert(data.GetCount() == 3);  // Should still be 3 (circular)
        
        std::cout << "  ✓ Circular buffer tests passed\n";
    }

    /**
     * @brief Test data aggregation between levels
     */
    static void TestPerfIntvlDataAggregation()
    {
        std::cout << "Testing PerfIntvlData aggregation...\n";
        
        PerfIntvlData higher(2, nullptr);
        PerfIntvlData lower(2, &higher);
        
        // Fill lower level to trigger aggregation
        lower.AddData(10.0, 5.0, 15.0);
        lower.AddData(20.0, 10.0, 30.0);
        
        // This should trigger aggregation to higher level
        lower.AddData(30.0, 15.0, 45.0);
        
        // Check that higher level received aggregated data
        assert(higher.GetCount() == 1);
        
        std::cout << "  ✓ Aggregation tests passed\n";
    }

    /**
     * @brief Test PerfProfMgr memory management
     */
    static void TestPerfProfMgrMemoryManagement()
    {
        std::cout << "Testing PerfProfMgr memory management...\n";
        
        PerfProfMgr mgr;
        
        // Test creating sections
        PERF_prof_sect *sect1 = mgr.NewSection("test1");
        PERF_prof_sect *sect2 = mgr.NewSection("test2");
        PERF_prof_sect *sect1_again = mgr.NewSection("test1");
        
        // Should return the same pointer for same ID
        assert(sect1 == sect1_again);
        assert(sect1 != sect2);
        
        // Test null input
        PERF_prof_sect *null_sect = mgr.NewSection(nullptr);
        assert(null_sect == nullptr);
        
        std::cout << "  ✓ Memory management tests passed\n";
    }

    /**
     * @brief Test report generation
     */
    static void TestPerfProfMgrReportGeneration()
    {
        std::cout << "Testing PerfProfMgr report generation...\n";
        
        PerfProfMgr mgr;
        char buffer[1024];
        
        // Test empty report
        size_t len = mgr.ReprPulse(buffer, sizeof(buffer));
        assert(len > 0);
        assert(strlen(buffer) == len);
        
        // Test with null buffer
        size_t null_len = mgr.ReprPulse(nullptr, 100);
        assert(null_len == 0);
        
        // Test with zero size
        buffer[0] = 'X';  // Set a marker
        size_t zero_len = mgr.ReprPulse(buffer, 0);
        assert(zero_len == 0);
        assert(buffer[0] == '\0');  // Should be null terminated
        
        std::cout << "  ✓ Report generation tests passed\n";
    }

    /**
     * @brief Test buffer safety
     */
    static void TestBufferSafety()
    {
        std::cout << "Testing buffer safety...\n";
        
        char small_buffer[10];
        
        // Test PERF_repr with small buffer
        size_t len = PERF_repr(small_buffer, sizeof(small_buffer));
        assert(len < sizeof(small_buffer));
        assert(small_buffer[len] == '\0');
        
        // Test with size 1 (should only contain null terminator)
        char tiny_buffer[1];
        size_t tiny_len = PERF_repr(tiny_buffer, 1);
        assert(tiny_len == 0);
        assert(tiny_buffer[0] == '\0');
        
        std::cout << "  ✓ Buffer safety tests passed\n";
    }

    /**
     * @brief Test input validation
     */
    static void TestInputValidation()
    {
        std::cout << "Testing input validation...\n";
        
        // Test null pointer handling
        PERF_prof_sect *null_ptr = nullptr;
        PERF_prof_sect_init(&null_ptr, nullptr);  // Should not crash
        
        PERF_prof_sect_enter(nullptr);  // Should not crash
        PERF_prof_sect_exit(nullptr);   // Should not crash
        
        // Test PERF_repr with null buffer
        size_t len = PERF_repr(nullptr, 100);
        assert(len == 0);
        
        std::cout << "  ✓ Input validation tests passed\n";
    }

    /**
     * @brief Test pulse logging functionality
     */
    static void TestPulseLogging()
    {
        std::cout << "Testing pulse logging...\n";
        
        // Test normal pulse logging
        PERF_log_pulse(50.0);
        PERF_log_pulse(75.0);
        PERF_log_pulse(125.0);  // Over 100%
        
        // Generate a report to verify data was logged
        char buffer[2048];
        size_t len = PERF_repr(buffer, sizeof(buffer));
        assert(len > 0);
        
        // Should contain pulse information
        assert(strstr(buffer, "Pulse") != nullptr);
        
        std::cout << "  ✓ Pulse logging tests passed\n";
    }

    /**
     * @brief Test threshold tracking
     */
    static void TestThresholdTracking()
    {
        std::cout << "Testing threshold tracking...\n";
        
        // Log some pulses that exceed thresholds
        PERF_log_pulse(150.0);  // Should exceed 100% threshold
        PERF_log_pulse(300.0);  // Should exceed 250% threshold
        
        // Generate report and check for threshold information
        char buffer[2048];
        size_t len = PERF_repr(buffer, sizeof(buffer));
        assert(len > 0);
        
        // Should contain threshold information
        assert(strstr(buffer, "Over") != nullptr);
        
        std::cout << "  ✓ Threshold tracking tests passed\n";
    }
};

/**
 * @brief Main test function
 */
int main()
{
    try
    {
        PerfMonTest::RunAllTests();
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
