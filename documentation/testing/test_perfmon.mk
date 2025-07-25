# Makefile for perfmon unit tests
# This can be included in the main Makefile or run standalone

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g -O0 -DTEST_BUILD
INCLUDES = -I.

# Test executable
TEST_PERFMON = test_perfmon

# Test source files
TEST_PERFMON_SRCS = test_perfmon.cpp

# Build and run perfmon tests
test-perfmon: $(TEST_PERFMON)
	@echo "Running perfmon unit tests..."
	./$(TEST_PERFMON)

# Build test executable
$(TEST_PERFMON): $(TEST_PERFMON_SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

# Clean test artifacts
clean-test-perfmon:
	rm -f $(TEST_PERFMON)

.PHONY: test-perfmon clean-test-perfmon
