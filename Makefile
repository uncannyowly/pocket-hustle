# Pocket Hustle
#
#   make test            unit tests, golden transcript, copy check (T=<module> runs one module)
#   make text            terminal version: build/host/ph-text
#   make sim             balance simulator: build/host/ph-sim
#   make game            PC version: build/host/pocket-hustle (needs SDL2)
#   make smoke           scripted headless run of the game binary
#   make shots           screenshots of every screen: build/shots/*.png
#   make miyoo           the Miyoo binary build/miyoo/pocket-hustle (inside the toolchain container; scripts/build-miyoo.sh)
#   make arm-check       compile and link the tests for the Miyoo (Docker toolchain image)
#   make emu             play every round (tests/golden/rounds) on the Miyoo build under qemu (scripts/emu-play.sh)
#   make cross-arch      play every round on the PC and the Miyoo build and compare the saves byte for byte
#   make fbview         the host viewer of the emulator's framebuffer file: build/host/fbview (needs SDL2)

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
GAME_SRC = $(sort $(wildcard src/app/*.c src/plat/plat_sdl.c))
MIYOO_SRC = $(sort $(wildcard src/app/*.c src/plat/plat_miyoo.c))
SHOTS_SRC = $(sort $(wildcard tools/shots/*.c))
SDL_CFLAGS = $(shell pkg-config --cflags sdl2)
SDL_LIBS = $(shell pkg-config --libs sdl2)

.PHONY: miyoo all test text sim game smoke shots arm-check arm-check-inner emu cross-arch fbview clean

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

build/host/pocket-hustle: $(CORE_SRC) $(UI_SRC) $(GAME_SRC) $(HEADERS)
	@mkdir -p $(dir $@)
	$(HOST_CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $(CORE_SRC) $(UI_SRC) $(GAME_SRC) $(SDL_LIBS) -lm

build/host/ph-shots: $(CORE_SRC) $(UI_SRC) $(SHOTS_SRC) tests/helpers.c tests/ui_helpers.c $(HEADERS)
	@mkdir -p $(dir $@)
	$(HOST_CC) $(CFLAGS) -Itests -o $@ $(CORE_SRC) $(UI_SRC) $(SHOTS_SRC) tests/helpers.c tests/ui_helpers.c -lm

build/host/fbview: tools/fbview/main.c
	@mkdir -p $(dir $@)
	$(HOST_CC) -std=c99 -Wall -Wextra -Werror -O2 -D_POSIX_C_SOURCE=200809L $(SDL_CFLAGS) -o $@ $< $(SDL_LIBS)

game: build/host/pocket-hustle

fbview: build/host/fbview

emu:
	scripts/emu-play.sh --all

cross-arch:
	scripts/cross-arch.sh


smoke: build/host/pocket-hustle
	scripts/smoke.sh

shots: build/host/ph-shots
	scripts/shots.sh

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

miyoo: build/miyoo/pocket-hustle

build/miyoo/pocket-hustle: $(CORE_SRC) $(UI_SRC) $(MIYOO_SRC) $(HEADERS)
	@mkdir -p $(dir $@)
	$(MIYOO_CC) $(MIYOO_CFLAGS) -o $@ $(CORE_SRC) $(UI_SRC) $(MIYOO_SRC) -lm -ldl -lpthread

arm-check:
	scripts/arm-check.sh

arm-check-inner:
	@mkdir -p build/arm
	$(MIYOO_CC) $(MIYOO_CFLAGS) -Itests -Itools/sim -o build/arm/run-tests $(CORE_SRC) $(UI_SRC) $(TEST_SRC) -lm

clean:
	rm -rf build
