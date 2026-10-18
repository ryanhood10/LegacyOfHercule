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
# A16_SPRITE: sprite id of Android 16's fighting record (tools/a16sprites.py, always built from the ROM);
# A16_TEST=1: player 1's L + R + Down spawns the Android 16 boss beside player 1 (src/coop/android16.c)
A16_SPRITE ?= 160
# STORY_TEST=1: chapter-test build (docs/testing.md "Story chapters"): the fork's TCP pad drives player 1
# (player 2 absent), the ROM block at 0xC00000 holds a story-flag preset (tools/fork/st_preset.py), player 1's
# L + R + Up wins every boss fight on the map (L + R + Left toggles the Android 16 autopilot with A16_TEST)
CFLAGS  := -mthumb -mcpu=arm7tdmi -Os -Wall -Wextra -ffreestanding -fno-builtin -fomit-frame-pointer \
           -ffunction-sections -fdata-sections -std=c11 -DP2_MODE=$(P2_MODE) -DWIDE_VIEW=$(WIDE) -DPARTY_SLOTS=$(PARTY_SLOTS) $(if $(filter 1,$(SSJ_TEST)),-DSSJ_TEST=1,) $(if $(filter 1,$(A16_TEST)),-DA16_TEST=1,) $(if $(filter 1,$(STORY_TEST)),-DSTORY_TEST=1,) -DA16_SPRITE=$(A16_SPRITE) $(if $(WIDE_TEST),-DWIDE_TEST=$(WIDE_TEST),) $(CFLAGS_EXTRA)
LDFLAGS := -nostdlib -nostartfiles -T src/coop/coop.ld -Wl,--gc-sections -Wl,-Map,$(BUILD)/coop.map

SRCS := $(wildcard src/coop/*.c)
OBJS := $(patsubst src/coop/%.c,$(BUILD)/obj/%.o,$(SRCS)) $(BUILD)/obj/sprites_data.o $(BUILD)/obj/a16_sprites_data.o
SSJ_TEST ?= 0

# BUU=1 (default when roms/buu.gba is present): sprites converted from Buu's Fury by tools/buu2log2.py,
# appended to the relocated sprite index. BUU_SPRITES lists "<log2 id>=<buu record>" pairs
# (158 = Hercule in his Buu-saga gi, 159 = Legendary Super Saiyan Broly; see docs/sprites.md).
# SSJ_SPRITE: the sprite the Super Saiyan Hercule row uses (test builds point it at 158/159).
comma := ,
empty :=
space := $(empty) $(empty)
BUU_ROM ?= roms/buu.gba
BUU ?= $(if $(wildcard $(BUU_ROM)),1,0)
BUU_SPRITES ?= 158=128 159=28
SSJ_SPRITE ?= 157
EXTRA_SPRITES := $(A16_SPRITE)=a16_record
ifeq ($(BUU),1)
OBJS += $(BUILD)/obj/buu_sprites_data.o
EXTRA_SPRITES += $(foreach s,$(BUU_SPRITES),$(firstword $(subst =, ,$(s)))=buu_record_$(firstword $(subst =, ,$(s))))
endif
CLONE_FLAGS := --extra $(subst $(space),$(comma),$(strip $(EXTRA_SPRITES)))
CLONE_FLAGS += --row-sprite $(SSJ_SPRITE)

all: $(OUT)

$(BUILD)/obj/%.o: src/coop/%.c src/coop/rom.h src/coop/coop.h src/coop/android16.h
	@mkdir -p $(BUILD)/obj
	$(CC) $(CFLAGS) -c $< -o $@

# Super Saiyan Hercule sprite + relocated sprite index / display rows, generated from the ROM (never committed)
$(BUILD)/gen/sprites_data.c: tools/spriteclone.py tools/sprites.py tools/mapchunks.py $(ROM) $(BUILD)/.spriteflags
	$(PYTHON) tools/spriteclone.py $(ROM) $@ $(CLONE_FLAGS)

# re-generate the index when the flags change (BUU, BUU_SPRITES, SSJ_SPRITE)
$(BUILD)/.spriteflags: FORCE
	@mkdir -p $(BUILD)
	@echo "$(CLONE_FLAGS) $(BUU_SPRITES)" | cmp -s - $@ || echo "$(CLONE_FLAGS) $(BUU_SPRITES)" > $@
FORCE:

# Buu's Fury records converted to LoG2 format (never committed); preview PNGs and a colour report beside them
$(BUILD)/gen/buu_sprites_data.c: tools/buu2log2.py tools/buu_sprites.py tools/sprites.py tools/mapchunks.py $(ROM) $(BUU_ROM) $(BUILD)/.spriteflags
	$(PYTHON) tools/buu2log2.py $(ROM) $(BUU_ROM) $@ $(BUU_SPRITES) --preview-dir $(BUILD) --report $(BUILD)/gen/buu_sprites_report.md

$(BUILD)/obj/buu_sprites_data.o: $(BUILD)/gen/buu_sprites_data.c src/coop/rom.h
	@mkdir -p $(BUILD)/obj
	$(CC) $(CFLAGS) -Isrc/coop -c $< -o $@

# Android 16's fighting sprite and the rocket / explosion projectile frames (never committed)
$(BUILD)/gen/a16_sprites_data.c: tools/a16sprites.py tools/buu2log2.py tools/buu_sprites.py tools/sprites.py tools/mapchunks.py $(ROM)
	$(PYTHON) tools/a16sprites.py $(ROM) $@ --preview $(BUILD)/a16_preview.png

$(BUILD)/obj/a16_sprites_data.o: $(BUILD)/gen/a16_sprites_data.c src/coop/rom.h
	@mkdir -p $(BUILD)/obj
	$(CC) $(CFLAGS) -Isrc/coop -c $< -o $@

$(BUILD)/obj/sprites_data.o: $(BUILD)/gen/sprites_data.c src/coop/rom.h
	@mkdir -p $(BUILD)/obj
	$(CC) $(CFLAGS) -Isrc/coop -c $< -o $@

$(BUILD)/coop.elf: $(OBJS) src/coop/coop.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -lgcc -o $@

$(BUILD)/coop.bin: $(BUILD)/coop.elf
	$(OBJCOPY) -O binary -j .text $< $@

$(BUILD)/coop.sym: $(BUILD)/coop.elf
	$(NM) $< > $@

$(BUILD)/hercule.patch.json: data/hercule.json $(wildcard data/*.json data/story/*.json data/story/*.lscript) tools/romdata.py tools/mapedit.py tools/script_asm.py
	$(PYTHON) tools/romdata.py build $< $@ $(ROM) --party-slots $(PARTY_SLOTS)

$(OUT): $(BUILD)/coop.bin $(BUILD)/coop.sym tools/patch_rom.py $(DATA_PATCH)
	$(PYTHON) tools/patch_rom.py $(ROM) $(BUILD)/coop.bin $(BUILD)/coop.sym $@ $(PATCH_FLAGS)

clean:
	rm -rf $(BUILD)/obj $(BUILD)/coop.* $(BUILD)/hercule.patch.json $(OUT)

.PHONY: all clean FORCE
