# Pocket Hustle
#
#   make test            unit tests, golden transcript, copy check (T=<module> runs one module)
#   make text            terminal version: build/host/ph-text
#   make sim             balance simulator: build/host/ph-sim
#   make arm-check       compile and link the tests for the Miyoo (Docker toolchain image)

CFLAGS := -std=c99 -Wall -Wextra -Werror -O2 -g -Isrc -D_POSIX_C_SOURCE=200809L
HOST_CC ?= cc
MIYOO_CC ?= arm-linux-gnueabihf-gcc
MIYOO_CFLAGS := $(CFLAGS) -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard

CORE_SRC = $(sort $(wildcard src/core/*.c src/content/*.c))
UI_SRC = $(sort $(wildcard src/gfx/*.c src/sfx/*.c src/ui/*.c))
HEADERS = $(wildcard src/core/*.h src/content/*.h src/gfx/*.h src/sfx/*.h src/ui/*.h src/plat/*.h src/text/*.h tests/*.h tools/sim/*.h)
TEST_SRC = $(sort $(wildcard tests/*.c)) $(wildcard tools/sim/report.c tools/sim/bots.c)
TEXT_SRC = $(sort $(wildcard src/text/*.c))
SIM_SRC = $(sort $(wildcard tools/sim/*.c))

.PHONY: all test text sim arm-check arm-check-inner clean

all: build/host/run-tests

build/host/run-tests: $(CORE_SRC) $(UI_SRC) $(TEST_SRC) $(HEADERS)
	@mkdir -p $(dir $@)
	$(HOST_CC) $(CFLAGS) -Itests -Itools/sim -o $@ $(CORE_SRC) $(UI_SRC) $(TEST_SRC) -lm

build/host/ph-text: $(CORE_SRC) $(TEXT_SRC) $(HEADERS)
	@mkdir -p $(dir $@)
	$(HOST_CC) $(CFLAGS) -o $@ $(CORE_SRC) $(TEXT_SRC)

build/host/ph-sim: $(CORE_SRC) $(SIM_SRC) $(HEADERS)
	@mkdir -p $(dir $@)
	$(HOST_CC) $(CFLAGS) -Itools/sim -o $@ $(CORE_SRC) $(SIM_SRC) -lm

text: build/host/ph-text

sim: build/host/ph-sim

test: build/host/run-tests
ifeq ($(strip $(T)),)
	build/host/run-tests
	@if [ -x tests/golden/run.sh ]; then tests/golden/run.sh; fi
	scripts/check-copy.sh
else
	build/host/run-tests $(T)
endif

arm-check:
	scripts/arm-check.sh

arm-check-inner:
	@mkdir -p build/arm
	$(MIYOO_CC) $(MIYOO_CFLAGS) -Itests -Itools/sim -o build/arm/run-tests $(CORE_SRC) $(UI_SRC) $(TEST_SRC) -lm

clean:
	rm -rf build
