CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
NM      := arm-none-eabi-nm
PYTHON  := python -I

BUILD   := build
ROM     := roms/log2.gba
# WIDE=1: dynamic wide view for the mGBA fork (tools/mgba-wide); output build/log2-wide.gba
WIDE ?= 0
WIDE_TEST ?=
OUT     := $(if $(filter 1,$(WIDE)),$(BUILD)/log2-wide.gba,$(BUILD)/log2-coop.gba)

# P2_MODE: 0 = link cable (default), 1 = mirror P1 (test), 2 = Select swaps the single controller
P2_MODE ?= 0
# DEBUG_MENU=1 patches the title screen to open the game's leftover debug menu (Map Test) after 30 s idle
DEBUG_MENU ?= 0
# PARTY_SLOTS: party entries (stock 6). The block is relocated to EWRAM whatever the value; > 6 also
# grows the save layout and patches every ROM site that assumed six (tools/patch_rom.py --party-slots)
PARTY_SLOTS ?= 8
# HERCULE=1 (default) applies the Legacy of Hercule data edits (data/hercule.json via tools/romdata.py)
HERCULE ?= 1
DATA_PATCH := $(if $(filter 1,$(HERCULE)),$(BUILD)/hercule.patch.json,)
PATCH_FLAGS := --party-slots $(PARTY_SLOTS) $(if $(filter 1,$(DEBUG_MENU)),--debug-menu,) $(if $(filter 1,$(WIDE)),--wide,) $(if $(DATA_PATCH),--data-patch $(DATA_PATCH),)
CFLAGS  := -mthumb -mcpu=arm7tdmi -Os -Wall -Wextra -ffreestanding -fno-builtin -fomit-frame-pointer \
           -ffunction-sections -fdata-sections -std=c11 -DP2_MODE=$(P2_MODE) -DWIDE_VIEW=$(WIDE) -DPARTY_SLOTS=$(PARTY_SLOTS) $(if $(filter 1,$(SSJ_TEST)),-DSSJ_TEST=1,) $(if $(WIDE_TEST),-DWIDE_TEST=$(WIDE_TEST),)
LDFLAGS := -nostdlib -nostartfiles -T src/coop/coop.ld -Wl,--gc-sections -Wl,-Map,$(BUILD)/coop.map

SRCS := $(wildcard src/coop/*.c)
OBJS := $(patsubst src/coop/%.c,$(BUILD)/obj/%.o,$(SRCS)) $(BUILD)/obj/sprites_data.o
SSJ_TEST ?= 0

all: $(OUT)

$(BUILD)/obj/%.o: src/coop/%.c src/coop/rom.h src/coop/coop.h
	@mkdir -p $(BUILD)/obj
	$(CC) $(CFLAGS) -c $< -o $@

# Super Saiyan Hercule sprite + relocated sprite index / display rows, generated from the ROM (never committed)
$(BUILD)/gen/sprites_data.c: tools/spriteclone.py tools/sprites.py tools/mapchunks.py $(ROM)
	$(PYTHON) tools/spriteclone.py $(ROM) $@

$(BUILD)/obj/sprites_data.o: $(BUILD)/gen/sprites_data.c src/coop/rom.h
	@mkdir -p $(BUILD)/obj
	$(CC) $(CFLAGS) -Isrc/coop -c $< -o $@

$(BUILD)/coop.elf: $(OBJS) src/coop/coop.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -lgcc -o $@

$(BUILD)/coop.bin: $(BUILD)/coop.elf
	$(OBJCOPY) -O binary -j .text $< $@

$(BUILD)/coop.sym: $(BUILD)/coop.elf
	$(NM) $< > $@

$(BUILD)/hercule.patch.json: data/hercule.json $(wildcard data/*.json) tools/romdata.py
	$(PYTHON) tools/romdata.py build $< $@ $(ROM) --party-slots $(PARTY_SLOTS)

$(OUT): $(BUILD)/coop.bin $(BUILD)/coop.sym tools/patch_rom.py $(DATA_PATCH)
	$(PYTHON) tools/patch_rom.py $(ROM) $(BUILD)/coop.bin $(BUILD)/coop.sym $@ $(PATCH_FLAGS)

clean:
	rm -rf $(BUILD)/obj $(BUILD)/coop.* $(BUILD)/hercule.patch.json $(OUT)

.PHONY: all clean
