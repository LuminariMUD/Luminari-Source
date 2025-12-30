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

# Autopilot pathfinding test files (Phase 01, Session 03)
PATHFINDING_TEST_SRC = test_autopilot_pathfinding.c
PATHFINDING_TEST_OBJ = $(BUILD_DIR)/test_autopilot_pathfinding.o
PATHFINDING_TEST_EXE = autopilot_pathfinding_tests

# NPC pilot test files (Phase 01, Session 05)
PILOT_TEST_SRC = test_npc_pilot.c
PILOT_TEST_OBJ = $(BUILD_DIR)/test_npc_pilot.o
PILOT_TEST_EXE = npc_pilot_tests

# Schedule test files (Phase 01, Session 06)
SCHEDULE_TEST_SRC = test_schedule.c
SCHEDULE_TEST_OBJ = $(BUILD_DIR)/test_schedule.o
SCHEDULE_TEST_EXE = schedule_tests

# Waypoint cache test files (Phase 01, Session 02)
WAYPOINT_TEST_SRC = test_waypoint_cache.c
WAYPOINT_TEST_OBJ = $(BUILD_DIR)/test_waypoint_cache.o
WAYPOINT_TEST_EXE = test_waypoint_cache

# Vehicle structs test files (Phase 02, Session 01)
VEHICLE_STRUCTS_TEST_SRC = test_vehicle_structs.c
VEHICLE_STRUCTS_TEST_OBJ = $(BUILD_DIR)/test_vehicle_structs.o
VEHICLE_STRUCTS_TEST_EXE = vehicle_structs_tests

# Vehicle movement test files (Phase 02, Session 03)
VEHICLE_MOVEMENT_TEST_SRC = test_vehicle_movement.c
VEHICLE_MOVEMENT_TEST_OBJ = $(BUILD_DIR)/test_vehicle_movement.o
VEHICLE_MOVEMENT_TEST_EXE = vehicle_movement_tests

# Vehicle transport test files (Phase 02, Session 05)
VEHICLE_TRANSPORT_TEST_SRC = vehicle_transport_tests.c
VEHICLE_TRANSPORT_TEST_OBJ = $(BUILD_DIR)/vehicle_transport_tests.o
VEHICLE_TRANSPORT_TEST_EXE = vehicle_transport_tests

# Vehicle creation test files (Phase 02, Session 02)
VEHICLE_CREATION_TEST_SRC = test_vehicle_creation.c
VEHICLE_CREATION_TEST_OBJ = $(BUILD_DIR)/test_vehicle_creation.o
VEHICLE_CREATION_TEST_EXE = vehicle_creation_tests

# Vehicle commands test files (Phase 02, Session 04)
VEHICLE_COMMANDS_TEST_SRC = test_vehicle_commands.c
VEHICLE_COMMANDS_TEST_OBJ = $(BUILD_DIR)/test_vehicle_commands.o
VEHICLE_COMMANDS_TEST_EXE = vehicle_commands_tests

# Transport unified test files (Phase 02, Session 06)
TRANSPORT_UNIFIED_TEST_SRC = test_transport_unified.c
TRANSPORT_UNIFIED_TEST_OBJ = $(BUILD_DIR)/test_transport_unified.o
TRANSPORT_UNIFIED_TEST_EXE = transport_unified_tests

# Vehicle stress test files (Phase 02, Session 07)
VEHICLE_STRESS_TEST_SRC = vehicle_stress_test.c
VEHICLE_STRESS_TEST_OBJ = $(BUILD_DIR)/vehicle_stress_test.o
VEHICLE_STRESS_TEST_EXE = vehicle_stress_test

# Vessel wilderness rooms test files (Phase 03, Session 04)
WILDERNESS_ROOMS_TEST_SRC = test_vessel_wilderness_rooms.c
WILDERNESS_ROOMS_TEST_OBJ = $(BUILD_DIR)/test_vessel_wilderness_rooms.o
WILDERNESS_ROOMS_TEST_EXE = vessel_wilderness_rooms_tests

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
all: $(BUILD_DIR) $(VESSEL_TEST_EXE) $(STRESS_TEST_EXE) $(AUTOPILOT_TEST_EXE) $(PATHFINDING_TEST_EXE) $(PILOT_TEST_EXE) $(SCHEDULE_TEST_EXE) $(WAYPOINT_TEST_EXE) $(VEHICLE_STRUCTS_TEST_EXE) $(VEHICLE_MOVEMENT_TEST_EXE) $(VEHICLE_TRANSPORT_TEST_EXE) $(VEHICLE_CREATION_TEST_EXE) $(VEHICLE_COMMANDS_TEST_EXE) $(TRANSPORT_UNIFIED_TEST_EXE) $(WILDERNESS_ROOMS_TEST_EXE)

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

# Compile pathfinding tests
$(BUILD_DIR)/test_autopilot_pathfinding.o: test_autopilot_pathfinding.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile NPC pilot tests
$(BUILD_DIR)/test_npc_pilot.o: test_npc_pilot.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile schedule tests
$(BUILD_DIR)/test_schedule.o: test_schedule.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile waypoint cache tests
$(BUILD_DIR)/test_waypoint_cache.o: test_waypoint_cache.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile vehicle structs tests (Phase 02)
$(BUILD_DIR)/test_vehicle_structs.o: test_vehicle_structs.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile vehicle movement tests (Phase 02, Session 03)
$(BUILD_DIR)/test_vehicle_movement.o: test_vehicle_movement.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile vehicle transport tests (Phase 02, Session 05)
$(BUILD_DIR)/vehicle_transport_tests.o: vehicle_transport_tests.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile vehicle creation tests (Phase 02, Session 02)
$(BUILD_DIR)/test_vehicle_creation.o: test_vehicle_creation.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile vehicle commands tests (Phase 02, Session 04)
$(BUILD_DIR)/test_vehicle_commands.o: test_vehicle_commands.c CuTest.h
	$(CC) $(CFLAGS) -DTEST_VEHICLE_COMMANDS_STANDALONE -c $< -o $@

# Compile transport unified tests (Phase 02, Session 06)
$(BUILD_DIR)/test_transport_unified.o: test_transport_unified.c CuTest.h
	$(CC) $(CFLAGS) -DSTANDALONE_TEST -c $< -o $@

# Compile vehicle stress test (Phase 02, Session 07)
$(BUILD_DIR)/vehicle_stress_test.o: vehicle_stress_test.c CuTest.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile vessel wilderness rooms tests (Phase 03, Session 04)
$(BUILD_DIR)/test_vessel_wilderness_rooms.o: test_vessel_wilderness_rooms.c CuTest.h
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

# Link pathfinding tests
$(PATHFINDING_TEST_EXE): $(BUILD_DIR) $(CUTEST_OBJ) $(PATHFINDING_TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $(CUTEST_OBJ) $(PATHFINDING_TEST_OBJ) $(LDFLAGS)

# Link NPC pilot tests
$(PILOT_TEST_EXE): $(BUILD_DIR) $(CUTEST_OBJ) $(PILOT_TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $(CUTEST_OBJ) $(PILOT_TEST_OBJ) $(LDFLAGS)

# Link schedule tests
$(SCHEDULE_TEST_EXE): $(BUILD_DIR) $(CUTEST_OBJ) $(SCHEDULE_TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $(CUTEST_OBJ) $(SCHEDULE_TEST_OBJ) $(LDFLAGS)

# Link waypoint cache tests
$(WAYPOINT_TEST_EXE): $(BUILD_DIR) $(CUTEST_OBJ) $(WAYPOINT_TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $(CUTEST_OBJ) $(WAYPOINT_TEST_OBJ) $(LDFLAGS)

# Link vehicle structs tests (Phase 02)
$(VEHICLE_STRUCTS_TEST_EXE): $(BUILD_DIR) $(CUTEST_OBJ) $(VEHICLE_STRUCTS_TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $(CUTEST_OBJ) $(VEHICLE_STRUCTS_TEST_OBJ) $(LDFLAGS)

# Link vehicle movement tests (Phase 02, Session 03)
$(VEHICLE_MOVEMENT_TEST_EXE): $(BUILD_DIR) $(CUTEST_OBJ) $(VEHICLE_MOVEMENT_TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $(CUTEST_OBJ) $(VEHICLE_MOVEMENT_TEST_OBJ) $(LDFLAGS)

# Link vehicle transport tests (Phase 02, Session 05)
$(VEHICLE_TRANSPORT_TEST_EXE): $(BUILD_DIR) $(CUTEST_OBJ) $(VEHICLE_TRANSPORT_TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $(CUTEST_OBJ) $(VEHICLE_TRANSPORT_TEST_OBJ) $(LDFLAGS)

# Link vehicle creation tests (Phase 02, Session 02)
$(VEHICLE_CREATION_TEST_EXE): $(BUILD_DIR) $(CUTEST_OBJ) $(VEHICLE_CREATION_TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $(CUTEST_OBJ) $(VEHICLE_CREATION_TEST_OBJ) $(LDFLAGS)

# Link vehicle commands tests (Phase 02, Session 04)
$(VEHICLE_COMMANDS_TEST_EXE): $(BUILD_DIR) $(CUTEST_OBJ) $(VEHICLE_COMMANDS_TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $(CUTEST_OBJ) $(VEHICLE_COMMANDS_TEST_OBJ) $(LDFLAGS)

# Link transport unified tests (Phase 02, Session 06)
$(TRANSPORT_UNIFIED_TEST_EXE): $(BUILD_DIR) $(CUTEST_OBJ) $(TRANSPORT_UNIFIED_TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $(CUTEST_OBJ) $(TRANSPORT_UNIFIED_TEST_OBJ) $(LDFLAGS)

# Link vehicle stress test (Phase 02, Session 07)
$(VEHICLE_STRESS_TEST_EXE): $(BUILD_DIR) $(CUTEST_OBJ) $(VEHICLE_STRESS_TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $(CUTEST_OBJ) $(VEHICLE_STRESS_TEST_OBJ) $(LDFLAGS)

# Link vessel wilderness rooms tests (Phase 03, Session 04)
$(WILDERNESS_ROOMS_TEST_EXE): $(BUILD_DIR) $(CUTEST_OBJ) $(WILDERNESS_ROOMS_TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $(CUTEST_OBJ) $(WILDERNESS_ROOMS_TEST_OBJ) $(LDFLAGS)

# Run unit tests
test: $(VESSEL_TEST_EXE)
	./$(VESSEL_TEST_EXE)

# Run stress tests
stress: $(STRESS_TEST_EXE)
	./$(STRESS_TEST_EXE)

# Run autopilot tests
autopilot: $(AUTOPILOT_TEST_EXE)
	./$(AUTOPILOT_TEST_EXE)

# Run pathfinding tests
pathfinding: $(PATHFINDING_TEST_EXE)
	./$(PATHFINDING_TEST_EXE)

# Run NPC pilot tests
pilot: $(PILOT_TEST_EXE)
	./$(PILOT_TEST_EXE)

# Run schedule tests
schedule: $(SCHEDULE_TEST_EXE)
	./$(SCHEDULE_TEST_EXE)

# Run waypoint cache tests
waypoint: $(WAYPOINT_TEST_EXE)
	./$(WAYPOINT_TEST_EXE)

# Run vehicle structs tests (Phase 02)
vehicle: $(VEHICLE_STRUCTS_TEST_EXE)
	./$(VEHICLE_STRUCTS_TEST_EXE)

# Run vehicle movement tests (Phase 02, Session 03)
vehicle-movement: $(VEHICLE_MOVEMENT_TEST_EXE)
	./$(VEHICLE_MOVEMENT_TEST_EXE)

# Run vehicle transport tests (Phase 02, Session 05)
vehicle-transport: $(VEHICLE_TRANSPORT_TEST_EXE)
	./$(VEHICLE_TRANSPORT_TEST_EXE)

# Run vehicle creation tests (Phase 02, Session 02)
vehicle-creation: $(VEHICLE_CREATION_TEST_EXE)
	./$(VEHICLE_CREATION_TEST_EXE)

# Run vehicle commands tests (Phase 02, Session 04)
vehicle-commands: $(VEHICLE_COMMANDS_TEST_EXE)
	./$(VEHICLE_COMMANDS_TEST_EXE)

# Run transport unified tests (Phase 02, Session 06)
transport-unified: $(TRANSPORT_UNIFIED_TEST_EXE)
	./$(TRANSPORT_UNIFIED_TEST_EXE)

# Run vehicle stress test (Phase 02, Session 07)
vehicle-stress: $(VEHICLE_STRESS_TEST_EXE)
	./$(VEHICLE_STRESS_TEST_EXE)

# Run vessel wilderness rooms tests (Phase 03, Session 04)
wilderness-rooms: $(WILDERNESS_ROOMS_TEST_EXE)
	./$(WILDERNESS_ROOMS_TEST_EXE)

# Run with Valgrind
valgrind: $(VESSEL_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(VESSEL_TEST_EXE)

# Run stress test with Valgrind
valgrind-stress: $(STRESS_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes ./$(STRESS_TEST_EXE)

# Run autopilot tests with Valgrind
valgrind-autopilot: $(AUTOPILOT_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(AUTOPILOT_TEST_EXE)

# Run pathfinding tests with Valgrind
valgrind-pathfinding: $(PATHFINDING_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(PATHFINDING_TEST_EXE)

# Run NPC pilot tests with Valgrind
valgrind-pilot: $(PILOT_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(PILOT_TEST_EXE)

# Run schedule tests with Valgrind
valgrind-schedule: $(SCHEDULE_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(SCHEDULE_TEST_EXE)

# Run waypoint cache tests with Valgrind
valgrind-waypoint: $(WAYPOINT_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(WAYPOINT_TEST_EXE)

# Run vehicle structs tests with Valgrind (Phase 02)
valgrind-vehicle: $(VEHICLE_STRUCTS_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(VEHICLE_STRUCTS_TEST_EXE)

# Run vehicle transport tests with Valgrind (Phase 02, Session 05)
valgrind-vehicle-transport: $(VEHICLE_TRANSPORT_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(VEHICLE_TRANSPORT_TEST_EXE)

# Run vehicle creation tests with Valgrind (Phase 02, Session 02)
valgrind-vehicle-creation: $(VEHICLE_CREATION_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(VEHICLE_CREATION_TEST_EXE)

# Run vehicle commands tests with Valgrind (Phase 02, Session 04)
valgrind-vehicle-commands: $(VEHICLE_COMMANDS_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(VEHICLE_COMMANDS_TEST_EXE)

# Run transport unified tests with Valgrind (Phase 02, Session 06)
valgrind-transport-unified: $(TRANSPORT_UNIFIED_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(TRANSPORT_UNIFIED_TEST_EXE)

# Run vehicle stress test with Valgrind (Phase 02, Session 07)
valgrind-vehicle-stress: $(VEHICLE_STRESS_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(VEHICLE_STRESS_TEST_EXE)

# Run vessel wilderness rooms tests with Valgrind (Phase 03, Session 04)
valgrind-wilderness-rooms: $(WILDERNESS_ROOMS_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(WILDERNESS_ROOMS_TEST_EXE)

# Run all tests
test-all: test stress autopilot pathfinding pilot schedule waypoint vehicle vehicle-movement vehicle-transport vehicle-creation vehicle-commands transport-unified

# Run all Phase 01 tests (autopilot automation layer)
phase01-tests: $(AUTOPILOT_TEST_EXE) $(PATHFINDING_TEST_EXE) $(WAYPOINT_TEST_EXE) $(PILOT_TEST_EXE) $(SCHEDULE_TEST_EXE)
	@echo "========================================="
	@echo "Running Phase 01 Automation Layer Tests"
	@echo "========================================="
	@echo ""
	@echo "--- Autopilot Structs (14 tests) ---"
	./$(AUTOPILOT_TEST_EXE)
	@echo ""
	@echo "--- Pathfinding (30 tests) ---"
	./$(PATHFINDING_TEST_EXE)
	@echo ""
	@echo "--- Waypoint Cache (11 tests) ---"
	./$(WAYPOINT_TEST_EXE)
	@echo ""
	@echo "--- NPC Pilot (12 tests) ---"
	./$(PILOT_TEST_EXE)
	@echo ""
	@echo "--- Schedule (17 tests) ---"
	./$(SCHEDULE_TEST_EXE)
	@echo ""
	@echo "========================================="
	@echo "Phase 01 Tests Complete: 84 tests total"
	@echo "========================================="

# Run Phase 01 tests with Valgrind
valgrind-phase01: $(AUTOPILOT_TEST_EXE) $(PATHFINDING_TEST_EXE) $(WAYPOINT_TEST_EXE) $(PILOT_TEST_EXE) $(SCHEDULE_TEST_EXE)
	@echo "========================================="
	@echo "Valgrind: Phase 01 Automation Layer Tests"
	@echo "========================================="
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(AUTOPILOT_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(PATHFINDING_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(WAYPOINT_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(PILOT_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(SCHEDULE_TEST_EXE)
	@echo "========================================="
	@echo "Valgrind Phase 01: All tests passed"
	@echo "========================================="

# Run all Phase 02 tests (Simple Vehicle Support)
phase02-tests: $(VEHICLE_STRUCTS_TEST_EXE) $(VEHICLE_MOVEMENT_TEST_EXE) $(VEHICLE_TRANSPORT_TEST_EXE) $(VEHICLE_CREATION_TEST_EXE) $(VEHICLE_COMMANDS_TEST_EXE) $(TRANSPORT_UNIFIED_TEST_EXE)
	@echo "========================================="
	@echo "Running Phase 02 Simple Vehicle Tests"
	@echo "========================================="
	@echo ""
	@echo "--- Vehicle Structs (19 tests) ---"
	./$(VEHICLE_STRUCTS_TEST_EXE)
	@echo ""
	@echo "--- Vehicle Creation (27 tests) ---"
	./$(VEHICLE_CREATION_TEST_EXE)
	@echo ""
	@echo "--- Vehicle Movement (45 tests) ---"
	./$(VEHICLE_MOVEMENT_TEST_EXE)
	@echo ""
	@echo "--- Vehicle Commands (31 tests) ---"
	./$(VEHICLE_COMMANDS_TEST_EXE)
	@echo ""
	@echo "--- Vehicle Transport (14 tests) ---"
	./$(VEHICLE_TRANSPORT_TEST_EXE)
	@echo ""
	@echo "--- Transport Unified (15 tests) ---"
	./$(TRANSPORT_UNIFIED_TEST_EXE)
	@echo ""
	@echo "========================================="
	@echo "Phase 02 Tests Complete: 151 tests total"
	@echo "========================================="

# Run Phase 02 tests with Valgrind
valgrind-phase02: $(VEHICLE_STRUCTS_TEST_EXE) $(VEHICLE_MOVEMENT_TEST_EXE) $(VEHICLE_TRANSPORT_TEST_EXE) $(VEHICLE_CREATION_TEST_EXE) $(VEHICLE_COMMANDS_TEST_EXE) $(TRANSPORT_UNIFIED_TEST_EXE)
	@echo "========================================="
	@echo "Valgrind: Phase 02 Simple Vehicle Tests"
	@echo "========================================="
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(VEHICLE_STRUCTS_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(VEHICLE_CREATION_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(VEHICLE_MOVEMENT_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(VEHICLE_COMMANDS_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(VEHICLE_TRANSPORT_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./$(TRANSPORT_UNIFIED_TEST_EXE)
	@echo "========================================="
	@echo "Valgrind Phase 02: All tests passed"
	@echo "========================================="

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(VESSEL_TEST_EXE)
	rm -f $(STRESS_TEST_EXE)
	rm -f $(AUTOPILOT_TEST_EXE)
	rm -f $(PATHFINDING_TEST_EXE)
	rm -f $(PILOT_TEST_EXE)
	rm -f $(SCHEDULE_TEST_EXE)
	rm -f $(WAYPOINT_TEST_EXE)
	rm -f $(VEHICLE_STRUCTS_TEST_EXE)
	rm -f $(VEHICLE_MOVEMENT_TEST_EXE)
	rm -f $(VEHICLE_TRANSPORT_TEST_EXE)
	rm -f $(VEHICLE_CREATION_TEST_EXE)
	rm -f $(VEHICLE_COMMANDS_TEST_EXE)
	rm -f $(TRANSPORT_UNIFIED_TEST_EXE)
	rm -f $(VEHICLE_STRESS_TEST_EXE)
	rm -f $(WILDERNESS_ROOMS_TEST_EXE)
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
	@echo "  all               - Build all tests (default)"
	@echo "  test              - Run vessel unit tests"
	@echo "  stress            - Run stress tests"
	@echo "  phase01-tests     - Run all Phase 01 tests (84 tests)"
	@echo "  phase02-tests     - Run all Phase 02 tests (151 tests)"
	@echo "  valgrind-phase01  - Run Phase 01 tests with Valgrind"
	@echo "  valgrind-phase02  - Run Phase 02 tests with Valgrind"
	@echo "  valgrind          - Run vessel tests with Valgrind"
	@echo "  valgrind-stress   - Run stress tests with Valgrind"
	@echo "  test-all          - Run all tests"
	@echo "  clean             - Remove build artifacts"
	@echo "  rebuild           - Clean and rebuild"
	@echo "  help              - Show this help"

.PHONY: all test stress autopilot pathfinding pilot schedule waypoint vehicle vehicle-movement vehicle-transport vehicle-creation vehicle-commands transport-unified vehicle-stress wilderness-rooms valgrind valgrind-stress valgrind-autopilot valgrind-pathfinding valgrind-pilot valgrind-schedule valgrind-waypoint valgrind-vehicle valgrind-vehicle-transport valgrind-vehicle-creation valgrind-vehicle-commands valgrind-transport-unified valgrind-vehicle-stress valgrind-wilderness-rooms phase01-tests valgrind-phase01 phase02-tests valgrind-phase02 test-all clean rebuild help
