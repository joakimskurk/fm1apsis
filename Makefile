CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
SAN_CFLAGS ?= -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined
INC = -Iapsis
SRC = apsis/apsis_state.c apsis/apsis_harmony.c apsis/apsis_gravity.c apsis/apsis_string.c apsis/apsis_drift.c apsis/apsis_clock.c apsis/apsis_spiral.c apsis/apsis_bloom.c apsis/apsis_keep.c apsis/apsis_chain.c apsis/apsis_mother.c apsis/apsis_transport.c apsis/apsis_late_binding.c apsis/apsis_runtime_final.c
OBJ = $(patsubst apsis/%.c,build/%.o,$(SRC))
SAN_OBJ = $(patsubst apsis/%.c,build/san_%.o,$(SRC))

.PHONY: all test sanitize clean
all: test

build:
	mkdir -p build

build/%.o: apsis/%.c | build
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

build/san_%.o: apsis/%.c | build
	$(CC) $(SAN_CFLAGS) $(INC) -c $< -o $@

build/test_apsis_core: $(OBJ) tests/test_apsis_core.c | build
	$(CC) $(CFLAGS) $(INC) $(OBJ) tests/test_apsis_core.c -o $@

build/test_gravity_bloom: $(OBJ) tests/test_gravity_bloom.c | build
	$(CC) $(CFLAGS) $(INC) $(OBJ) tests/test_gravity_bloom.c -o $@

build/test_keep_chain_mother: $(OBJ) tests/test_keep_chain_mother.c | build
	$(CC) $(CFLAGS) $(INC) $(OBJ) tests/test_keep_chain_mother.c -o $@

build/test_transport: $(OBJ) tests/test_transport.c | build
	$(CC) $(CFLAGS) $(INC) $(OBJ) tests/test_transport.c -o $@

build/test_runtime: $(OBJ) tests/test_runtime.c | build
	$(CC) $(CFLAGS) $(INC) $(OBJ) tests/test_runtime.c -o $@

build/test_runtime_lifecycle: $(OBJ) tests/test_runtime_lifecycle.c | build
	$(CC) $(CFLAGS) $(INC) $(OBJ) tests/test_runtime_lifecycle.c -o $@

build/test_runtime_extreme: $(OBJ) tests/test_runtime_extreme.c | build
	$(CC) $(CFLAGS) $(INC) $(OBJ) tests/test_runtime_extreme.c -o $@

build/test_late_binding: $(OBJ) tests/test_late_binding.c | build
	$(CC) $(CFLAGS) $(INC) $(OBJ) tests/test_late_binding.c -o $@

build/san_apsis_core: $(SAN_OBJ) tests/test_apsis_core.c | build
	$(CC) $(SAN_CFLAGS) $(INC) $(SAN_OBJ) tests/test_apsis_core.c -o $@

build/san_gravity_bloom: $(SAN_OBJ) tests/test_gravity_bloom.c | build
	$(CC) $(SAN_CFLAGS) $(INC) $(SAN_OBJ) tests/test_gravity_bloom.c -o $@

build/san_keep_chain_mother: $(SAN_OBJ) tests/test_keep_chain_mother.c | build
	$(CC) $(SAN_CFLAGS) $(INC) $(SAN_OBJ) tests/test_keep_chain_mother.c -o $@

build/san_transport: $(SAN_OBJ) tests/test_transport.c | build
	$(CC) $(SAN_CFLAGS) $(INC) $(SAN_OBJ) tests/test_transport.c -o $@

build/san_runtime: $(SAN_OBJ) tests/test_runtime.c | build
	$(CC) $(SAN_CFLAGS) $(INC) $(SAN_OBJ) tests/test_runtime.c -o $@

build/san_runtime_lifecycle: $(SAN_OBJ) tests/test_runtime_lifecycle.c | build
	$(CC) $(SAN_CFLAGS) $(INC) $(SAN_OBJ) tests/test_runtime_lifecycle.c -o $@

build/san_runtime_extreme: $(SAN_OBJ) tests/test_runtime_extreme.c | build
	$(CC) $(SAN_CFLAGS) $(INC) $(SAN_OBJ) tests/test_runtime_extreme.c -o $@

build/san_late_binding: $(SAN_OBJ) tests/test_late_binding.c | build
	$(CC) $(SAN_CFLAGS) $(INC) $(SAN_OBJ) tests/test_late_binding.c -o $@

test: build/test_apsis_core build/test_gravity_bloom build/test_keep_chain_mother build/test_transport build/test_runtime build/test_runtime_lifecycle build/test_runtime_extreme build/test_late_binding
	build/test_apsis_core
	build/test_gravity_bloom
	build/test_keep_chain_mother
	build/test_transport
	build/test_runtime
	build/test_runtime_lifecycle
	build/test_runtime_extreme
	build/test_late_binding

sanitize: build/san_apsis_core build/san_gravity_bloom build/san_keep_chain_mother build/san_transport build/san_runtime build/san_runtime_lifecycle build/san_runtime_extreme build/san_late_binding
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/san_apsis_core
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/san_gravity_bloom
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/san_keep_chain_mother
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/san_transport
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/san_runtime
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/san_runtime_lifecycle
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/san_runtime_extreme
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/san_late_binding

clean:
	rm -rf build
