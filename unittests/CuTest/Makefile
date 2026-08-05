# Focused standalone test harnesses for LuminariMUD.
#
# Production-linked tests live in the root Autotools target. Transport,
# autopilot, and vehicle behavior must be tested there so the tests exercise
# the implementation shipped by the game server.

CC = gcc
C23_TEST_SOURCE = bool value=true; void *ptr=nullptr; static_assert(sizeof(bool)>0);
CSTD_FLAG := $(shell \
	if printf '%s\n' '$(C23_TEST_SOURCE)' | \
	   $(CC) -std=gnu23 -x c -fsyntax-only - >/dev/null 2>&1; then \
	  printf '%s' -std=gnu23; \
	elif printf '%s\n' '$(C23_TEST_SOURCE)' | \
	     $(CC) -std=gnu2x -x c -fsyntax-only - >/dev/null 2>&1; then \
	  printf '%s' -std=gnu2x; \
	fi)
ifeq ($(strip $(CSTD_FLAG)),)
$(error $(CC) does not provide the required GNU C23 language support)
endif

PROTOCOL_TEST_CFLAGS ?= -Wall -Wextra $(CSTD_FLAG) -g -O0 \
	-ffunction-sections -fdata-sections
PROTOCOL_TEST_LDFLAGS ?= -lm -ljson-c -Wl,--gc-sections
PROTOCOL_TEST_DEFINES = -DLUMINARI_PROTOCOL_TEST=1

BUILD_DIR = build
CUTEST_OBJ = $(BUILD_DIR)/CuTest.o
PROTOCOL_PARSER_TEST_OBJ = $(BUILD_DIR)/test_protocol_parser.o
PROTOCOL_SOURCE_OBJ = $(BUILD_DIR)/protocol.o
MSDP_JSON_SOURCE_OBJ = $(BUILD_DIR)/msdp_json.o
PROTOCOL_ONBOARDING_OBJ = $(BUILD_DIR)/onboarding.o
PROTOCOL_PARSER_TEST_EXE = protocol_parser_tests
PROTOCOL_FUZZ_OBJ = $(BUILD_DIR)/fuzz_protocol_parser.o
PROTOCOL_FUZZ_SOURCE_OBJ = $(BUILD_DIR)/protocol_fuzz.o
MSDP_JSON_FUZZ_SOURCE_OBJ = $(BUILD_DIR)/msdp_json_fuzz.o
PROTOCOL_FUZZ_EXE = protocol_parser_fuzz
FUZZ_CC ?= $(shell command -v clang 2>/dev/null || command -v clang-18 2>/dev/null)
FUZZ_SECONDS ?= 10
FUZZ_CFLAGS ?= -Wall -Wextra $(CSTD_FLAG) -g -O1 -fno-omit-frame-pointer \
	-fsanitize=fuzzer,address,undefined
FUZZ_LDFLAGS ?= -lm -ljson-c -fsanitize=fuzzer,address,undefined

all: $(PROTOCOL_PARSER_TEST_EXE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(CUTEST_OBJ): CuTest.c CuTest.h | $(BUILD_DIR)
	$(CC) $(PROTOCOL_TEST_CFLAGS) -c CuTest.c -o $@

$(PROTOCOL_PARSER_TEST_OBJ): test_protocol_parser.c CuTest.h \
		../../src/net/protocol.h ../../src/structs.h | $(BUILD_DIR)
	$(CC) $(PROTOCOL_TEST_CFLAGS) $(PROTOCOL_TEST_DEFINES) -I../../src \
		-c test_protocol_parser.c -o $@

$(PROTOCOL_SOURCE_OBJ): ../../src/net/protocol.c ../../src/net/protocol.h \
		../../src/net/msdp_json.h ../../src/structs.h ../../src/net/onboarding.h | $(BUILD_DIR)
	$(CC) $(PROTOCOL_TEST_CFLAGS) $(PROTOCOL_TEST_DEFINES) -I../../src \
		-c ../../src/net/protocol.c -o $@

$(MSDP_JSON_SOURCE_OBJ): ../../src/net/msdp_json.c ../../src/net/msdp_json.h \
		../../src/net/protocol.h ../../src/structs.h | $(BUILD_DIR)
	$(CC) $(PROTOCOL_TEST_CFLAGS) $(PROTOCOL_TEST_DEFINES) -I../../src \
		-c ../../src/net/msdp_json.c -o $@

$(PROTOCOL_ONBOARDING_OBJ): ../../src/net/onboarding.c \
		../../src/net/onboarding.h ../../src/structs.h | $(BUILD_DIR)
	$(CC) $(PROTOCOL_TEST_CFLAGS) -DWEB_ONBOARDING_FOCUSED_PROTOCOL_HARNESS=1 -I../../src \
		-c ../../src/net/onboarding.c -o $@

$(PROTOCOL_PARSER_TEST_EXE): $(CUTEST_OBJ) $(PROTOCOL_PARSER_TEST_OBJ) \
		$(PROTOCOL_SOURCE_OBJ) $(MSDP_JSON_SOURCE_OBJ) $(PROTOCOL_ONBOARDING_OBJ)
	$(CC) $(PROTOCOL_TEST_CFLAGS) -o $@ $^ $(PROTOCOL_TEST_LDFLAGS)

$(PROTOCOL_FUZZ_OBJ): fuzz_protocol_parser.c ../../src/net/protocol.h \
		../../src/structs.h ../../src/net/onboarding.h | $(BUILD_DIR)
	$(FUZZ_CC) $(FUZZ_CFLAGS) -I../../src -c fuzz_protocol_parser.c -o $@

$(PROTOCOL_FUZZ_SOURCE_OBJ): ../../src/net/protocol.c ../../src/net/protocol.h \
		../../src/net/msdp_json.h ../../src/structs.h ../../src/net/onboarding.h | $(BUILD_DIR)
	$(FUZZ_CC) $(FUZZ_CFLAGS) -I../../src -c ../../src/net/protocol.c -o $@

$(MSDP_JSON_FUZZ_SOURCE_OBJ): ../../src/net/msdp_json.c ../../src/net/msdp_json.h \
		../../src/net/protocol.h ../../src/structs.h | $(BUILD_DIR)
	$(FUZZ_CC) $(FUZZ_CFLAGS) -I../../src -c ../../src/net/msdp_json.c -o $@

$(PROTOCOL_FUZZ_EXE): $(PROTOCOL_FUZZ_OBJ) $(PROTOCOL_FUZZ_SOURCE_OBJ) \
		$(MSDP_JSON_FUZZ_SOURCE_OBJ)
	$(FUZZ_CC) $(FUZZ_CFLAGS) -o $@ $^ $(FUZZ_LDFLAGS)

production:
	$(MAKE) -C ../.. test

protocol-parser: $(PROTOCOL_PARSER_TEST_EXE)
	./$(PROTOCOL_PARSER_TEST_EXE)

protocol-fuzz-build: $(PROTOCOL_FUZZ_EXE)

protocol-fuzz: $(PROTOCOL_FUZZ_EXE)
	fuzz_work=$$(mktemp -d /tmp/luminari-protocol-fuzz.XXXXXX); \
	trap 'rm -rf "$$fuzz_work"' EXIT; \
	cp fuzz_corpus/* "$$fuzz_work/"; \
	ASAN_SYMBOLIZER_PATH=$$(command -v llvm-symbolizer 2>/dev/null || \
		command -v llvm-symbolizer-18 2>/dev/null || true) \
	ASAN_OPTIONS=halt_on_error=1 \
	UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
	./$(PROTOCOL_FUZZ_EXE) -max_total_time=$(FUZZ_SECONDS) -timeout=5 \
		-print_funcs=0 -verbosity=0 -max_len=16384 "$$fuzz_work"

test: production

test-all:
	$(MAKE) -C ../.. test-all

valgrind-protocol: $(PROTOCOL_PARSER_TEST_EXE)
	valgrind --leak-check=full --track-origins=yes --error-exitcode=1 \
		./$(PROTOCOL_PARSER_TEST_EXE)

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(PROTOCOL_PARSER_TEST_EXE) $(PROTOCOL_FUZZ_EXE)

rebuild: clean all

help:
	@echo "LuminariMUD test targets"
	@echo ""
	@echo "  all                - Build the focused protocol parser harness"
	@echo "  production         - Run the root production-linked CuTest suite"
	@echo "  protocol-parser    - Run the focused protocol parser harness"
	@echo "  protocol-fuzz      - Run bounded ASan/UBSan libFuzzer coverage"
	@echo "  test-all           - Run the authoritative root test-all target"
	@echo "  valgrind-protocol  - Run the protocol harness with Valgrind"
	@echo "  clean              - Remove standalone build artifacts"

.PHONY: all production protocol-parser protocol-fuzz-build protocol-fuzz test \
	test-all valgrind-protocol clean rebuild help
