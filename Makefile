CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
SAN_CFLAGS ?= -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined
INC = -Iapsis
SRC = apsis/apsis_state.c apsis/apsis_harmony.c apsis/apsis_string.c apsis/apsis_drift.c apsis/apsis_clock.c apsis/apsis_spiral.c
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

build/san_apsis_core: $(SAN_OBJ) tests/test_apsis_core.c | build
	$(CC) $(SAN_CFLAGS) $(INC) $(SAN_OBJ) tests/test_apsis_core.c -o $@

test: build/test_apsis_core
	build/test_apsis_core

sanitize: build/san_apsis_core
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/san_apsis_core

clean:
	rm -rf build
