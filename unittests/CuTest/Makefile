# Makefile for LuminariMUD Vessel Unit Tests
# Part of Phase 00, Session 09: Testing and Validation

CC = gcc
CFLAGS = -Wall -Wextra -std=c89 -pedantic -g -O0
LDFLAGS = -lm

# Directories
SRC_DIR = .
BUILD_DIR = build

# CuTest framework files
CUTEST_SRC = CuTest.c
CUTEST_OBJ = $(BUILD_DIR)/CuTest.o

# Vessel test files
VESSEL_TEST_SRCS = \
	test_vessels.c \
	test_vessel_coords.c \
	test_vessel_types.c \
	test_vessel_rooms.c \
	test_vessel_movement.c \
	test_vessel_persistence.c

VESSEL_TEST_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(VESSEL_TEST_SRCS))

# Autopilot test files (Phase 01)
AUTOPILOT_TEST_SRC = test_autopilot_structs.c
AUTOPILOT_TEST_OBJ = $(BUILD_DIR)/test_autopilot_structs.o
AUTOPILOT_TEST_EXE = autopilot_tests

# Stress test files
STRESS_TEST_SRC = vessel_stress_test.c
STRESS_TEST_OBJ = $(BUILD_DIR)/vessel_stress_test.o

# Test runner
VESSEL_RUNNER_SRC = vessel_test_runner.c
VESSEL_RUNNER_OBJ = $(BUILD_DIR)/vessel_test_runner.o

# Executables
VESSEL_TEST_EXE = vessel_tests
STRESS_TEST_EXE = vessel_stress_test
ALL_TESTS_EXE = test_runner

# Default target
all: $(BUILD_DIR) $(VESSEL_TEST_EXE) $(STRESS_TEST_EXE) $(AUTOPILOT_TEST_EXE)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile CuTest framework
$(BUILD_DIR)/CuTest.o: CuTest.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile vessel test files
$(BUILD_DIR)/test_vessels.o: test_vessels.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_vessel_coords.o: test_vessel_coords.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_vessel_types.o: test_vessel_types.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_vessel_rooms.o: test_vessel_rooms.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_vessel_movement.o: test_vessel_movement.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_vessel_persistence.o: test_vessel_persistence.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile test runner
$(BUILD_DIR)/vessel_test_runner.o: vessel_test_runner.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile stress test
$(BUILD_DIR)/vessel_stress_test.o: vessel_stress_test.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile autopilot tests
$(BUILD_DIR)/test_autopilot_structs.o: test_autopilot_structs.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

# Link vessel unit tests
$(VESSEL_TEST_EXE): $(BUILD_DIR) $(CUTEST_OBJ) $(VESSEL_TEST_OBJS) $(VESSEL_RUNNER_OBJ)
	$(CC) $(CFLAGS) -o $@ $(CUTEST_OBJ) $(VESSEL_TEST_OBJS) $(VESSEL_RUNNER_OBJ) $(LDFLAGS)

# Link stress test
$(STRESS_TEST_EXE): $(BUILD_DIR) $(STRESS_TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $(STRESS_TEST_OBJ) $(LDFLAGS)

# Link autopilot tests
$(AUTOPILOT_TEST_EXE): $(BUILD_DIR) $(CUTEST_OBJ) $(AUTOPILOT_TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $(CUTEST_OBJ) $(AUTOPILOT_TEST_OBJ) $(LDFLAGS)

# Run unit tests
test: $(VESSEL_TEST_EXE)
	./$(VESSEL_TEST_EXE)

# Run stress tests
stress: $(STRESS_TEST_EXE)
	./$(STRESS_TEST_EXE)

# Run autopilot tests
autopilot: $(AUTOPILOT_TEST_EXE)
	./$(AUTOPILOT_TEST_EXE)

# Run with Valgrind
valgrind: $(VESSEL_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(VESSEL_TEST_EXE)

# Run stress test with Valgrind
valgrind-stress: $(STRESS_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes ./$(STRESS_TEST_EXE)

# Run autopilot tests with Valgrind
valgrind-autopilot: $(AUTOPILOT_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(AUTOPILOT_TEST_EXE)

# Run all tests
test-all: test stress autopilot

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(VESSEL_TEST_EXE)
	rm -f $(STRESS_TEST_EXE)
	rm -f $(AUTOPILOT_TEST_EXE)
	rm -f *.o

# Rebuild from scratch
rebuild: clean all

# Generate AllTests.c using make-tests.sh (for legacy compatibility)
AllTests.c: $(wildcard test*.c)
	./make-tests.sh > AllTests.c

# Help target
help:
	@echo "LuminariMUD Vessel Test Suite Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all           - Build all tests (default)"
	@echo "  test          - Run unit tests"
	@echo "  stress        - Run stress tests"
	@echo "  valgrind      - Run unit tests with Valgrind"
	@echo "  valgrind-stress - Run stress tests with Valgrind"
	@echo "  test-all      - Run all tests"
	@echo "  clean         - Remove build artifacts"
	@echo "  rebuild       - Clean and rebuild"
	@echo "  help          - Show this help"

.PHONY: all test stress autopilot valgrind valgrind-stress valgrind-autopilot test-all clean rebuild help
