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
# grows the save layout and patches every ROM site that assumed six (tools/patch_rom.py --party-slots).
# 9 (default): slot 6 Krillin, 7 the duel clone's (Android 16's in his chapter), 8 Super Saiyan 2 Gohan
PARTY_SLOTS ?= 9
# HERCULE=1 (default) applies the Legacy of Hercule data edits (data/hercule.json via tools/romdata.py)
HERCULE ?= 1
DATA_PATCH := $(if $(filter 1,$(HERCULE)),$(BUILD)/hercule.patch.json,)
# MAPS=1 (default): new maps built from data/maps/*.json by tools/mapbuild.py (zone 15: arctic, Time
# Chamber, Broly's arena, the Buu crater; docs/ids.md, docs/maps.md), placed from MAP_FREE and appended to the relocated map table
MAPS ?= 1
MAP_SPECS ?= $(sort $(wildcard data/maps/*.json))
MAP_FREE ?= 0xD00000
MAPS_PATCH := $(if $(and $(filter 1,$(MAPS)),$(MAP_SPECS)),$(BUILD)/maps.patch.json,)
PATCH_FLAGS := --party-slots $(PARTY_SLOTS) $(if $(filter 1,$(DEBUG_MENU)),--debug-menu,) $(if $(filter 1,$(WIDE)),--wide,) $(if $(DATA_PATCH),--data-patch $(DATA_PATCH),) $(if $(MAPS_PATCH),--maps $(MAPS_PATCH),)
# A16_SPRITE: sprite id of Android 16's fighting record (tools/a16sprites.py, always built from the ROM);
# A16_TEST=1: player 1's L + R + Down spawns the Android 16 boss beside player 1 (src/coop/android16.c)
A16_SPRITE ?= 160
# A16P_SPRITE: Android 16 as a player (tools/a16sprites.py a16p_record; display row 25, party slot 7 in the
# "Gero's computer" chapter, src/coop/a16player.c)
A16P_SPRITE ?= 175
# STORY_TEST=1: chapter-test build (docs/testing.md "Story chapters"): the fork's TCP pad drives player 1
# (player 2 absent), the ROM block at 0xC00000 holds a story-flag preset (tools/fork/st_preset.py), player 1's
# L + R + Up wins every boss fight on the map (L + R + Left toggles the Android 16 autopilot with A16_TEST)
# KRILLIN_SPRITE: sprite id of Krillin's playable record (tools/krillinsprites.py, built from the ROM: his
# NPC set 76 plus Goku's fight groups re-headed); OLD_HERCULE_SPRITE: old Hercule's row 24 (158 = the Buu's
# Fury Hercule, 72 without roms/buu.gba). KR_TEST=1: player 1's L+R+Up toggles old Hercule on slot 5,
# L+R+Left sets Krillin's EXP one short of his next level (src/coop/krillin.c)
KRILLIN_SPRITE ?= 161
# SSJ2_TEST=1: player 2's pad L + R + Select joins Super Saiyan 2 Gohan (slot 8, src/coop/ssj2gohan.c), + Start makes
# him player 1, + Up player 2, + Down sets his EXP one short of his next level
# A13_SPRITE / NAMEK_SPRITE: Android 13 (base form) and Cooler's Namekian servant (tools/bossmodels.py,
# built from the ROM); src/coop/bosses.c gives them to the co-op twins of 19, Dr. Gero and Cooler
A13_SPRITE ?= 165
NAMEK_SPRITE ?= 166
# PARAGUS_SPRITE / SHIP_SPRITE / PARAGUS_PORTRAIT: the Broly side chapter's art (tools/brolysprites.py, built from
# the ROMs: Vegeta's set made old, Buu's Fury ship record 1, Vegeta's portrait made old); src/coop/bosses.c
# CELL2_SPRITE: a copy of Perfect Cell's stock record 30 (tools/spriteclone.py "@30"), so the Cell Games'
# scripts can address the second Cell apart
CELL2_SPRITE ?= 162
PARAGUS_SPRITE ?= 170
SHIP_SPRITE ?= 171
PARAGUS_PORTRAIT ?= 120
# HALO_GOKU_SPRITE / HALO_SSJ_SPRITE: Goku and Super Saiyan Goku with a halo on every frame (tools/halosprites.py,
# built from the ROM), display rows 26 / 27; a script makes Goku (slot 4) dead with SetCharacterTransformation(4, 176)
HALO_GOKU_SPRITE ?= 176
HALO_SSJ_SPRITE ?= 177
# S13_SPRITE / S13_PORTRAIT: Super Android 13, the post-game boss (tools/super13sprites.py: Buu's Fury Broly recoloured,
# Perfect Cell's groups as a stand-in with BUU=0; his ki orbs; Android 16's portrait made Super 13); src/coop/super13.c
S13_SPRITE ?= 180
S13_PORTRAIT ?= 122
CFLAGS  := -mthumb -mcpu=arm7tdmi -Os -Wall -Wextra -ffreestanding -fno-builtin -fomit-frame-pointer \
           -ffunction-sections -fdata-sections -std=c11 -DP2_MODE=$(P2_MODE) -DWIDE_VIEW=$(WIDE) -DPARTY_SLOTS=$(PARTY_SLOTS) $(if $(filter 1,$(SSJ_TEST)),-DSSJ_TEST=1,) $(if $(filter 1,$(A16_TEST)),-DA16_TEST=1,) -DA16_SPRITE=$(A16_SPRITE) -DA16P_SPRITE=$(A16P_SPRITE) -DA13_SPRITE=$(A13_SPRITE) -DNAMEK_SPRITE=$(NAMEK_SPRITE) -DPARAGUS_SPRITE=$(PARAGUS_SPRITE) -DCELL2_SPRITE=$(CELL2_SPRITE) $(if $(filter 1,$(KR_TEST)),-DKR_TEST=1,) $(if $(filter 1,$(STORY_TEST)),-DSTORY_TEST=1,) $(if $(filter 1,$(SSJ2_TEST)),-DSSJ2_TEST=1,) $(if $(WIDE_TEST),-DWIDE_TEST=$(WIDE_TEST),) $(CFLAGS_EXTRA) -DS13_SPRITE=$(S13_SPRITE)
LDFLAGS := -nostdlib -nostartfiles -T src/coop/coop.ld -Wl,--gc-sections -Wl,-Map,$(BUILD)/coop.map

SRCS := $(wildcard src/coop/*.c)
OBJS := $(patsubst src/coop/%.c,$(BUILD)/obj/%.o,$(SRCS)) $(BUILD)/obj/sprites_data.o $(BUILD)/obj/a16_sprites_data.o $(BUILD)/obj/krillin_sprites_data.o $(BUILD)/obj/boss_models_data.o $(BUILD)/obj/broly_sprites_data.o $(BUILD)/obj/hercule_sprites_data.o $(BUILD)/obj/halo_sprites_data.o $(BUILD)/obj/ssj2_sprites_data.o $(BUILD)/obj/super13_sprites_data.o

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
# 72=herc_record: Hercule's stock record plus the Kamehameha pose (group 18, tools/herculesprites.py) replaces id 72;
# 58=ssj2_record: Super Saiyan 2 Gohan's stock record with the hurt / lying / tumble groups (tools/ssj2sprites.py)
EXTRA_SPRITES := 72=herc_record 58=ssj2_record $(CELL2_SPRITE)=@30 $(A16_SPRITE)=a16_record $(A16P_SPRITE)=a16p_record $(KRILLIN_SPRITE)=krillin_record $(A13_SPRITE)=a13_record $(NAMEK_SPRITE)=namek_record $(PARAGUS_SPRITE)=paragus_record $(SHIP_SPRITE)=ship_record $(HALO_GOKU_SPRITE)=halo_goku_record $(HALO_SSJ_SPRITE)=halo_ssj_record $(HALO_SSJ_SPRITE)=halo_ssj_record $(S13_SPRITE)=s13_record

ifeq ($(BUU),1)
OBJS += $(BUILD)/obj/buu_sprites_data.o
EXTRA_SPRITES += $(foreach s,$(BUU_SPRITES),$(firstword $(subst =, ,$(s)))=buu_record_$(firstword $(subst =, ,$(s))))
endif
CLONE_FLAGS := --extra $(subst $(space),$(comma),$(strip $(EXTRA_SPRITES)))
CLONE_FLAGS += --row-sprite $(SSJ_SPRITE)
OLD_HERCULE_SPRITE ?= $(if $(filter 1,$(BUU)),158,72)
CLONE_FLAGS += --krillin-sprite $(KRILLIN_SPRITE) --old-hercule-sprite $(OLD_HERCULE_SPRITE) --a16-sprite $(A16P_SPRITE) --halo-sprites $(HALO_GOKU_SPRITE),$(HALO_SSJ_SPRITE)

all: $(OUT)

$(BUILD)/obj/%.o: src/coop/%.c src/coop/rom.h src/coop/coop.h src/coop/android16.h src/coop/krillin.h src/coop/bosses.h src/coop/a16player.h src/coop/ssj2gohan.h src/coop/super13.h

	@mkdir -p $(BUILD)/obj
	$(CC) $(CFLAGS) -DOLD_HERCULE_SPRITE=$(OLD_HERCULE_SPRITE) -c $< -o $@

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

# Krillin's playable sprite, status portrait, ki blast / Destructo Disc frames and icons, the relocated UI
# string table and portrait module table (never committed)
EXTRA_PORTRAITS := --extra-portrait $(PARAGUS_PORTRAIT)=paragus_portrait --extra-portrait $(S13_PORTRAIT)=s13_portrait
# the portrait list as a stamp: the table is regenerated when an extra portrait is added or renumbered
$(BUILD)/gen/portraits.stamp: FORCE
	@mkdir -p $(BUILD)/gen
	@echo '$(EXTRA_PORTRAITS)' | cmp -s - $@ || echo '$(EXTRA_PORTRAITS)' > $@

$(BUILD)/gen/krillin_sprites_data.c: tools/krillinsprites.py tools/buu2log2.py tools/buu_sprites.py tools/sprites.py tools/mapchunks.py $(ROM) $(BUILD)/gen/portraits.stamp
	$(PYTHON) tools/krillinsprites.py $(ROM) $@ --preview $(BUILD)/krillin_preview.png $(EXTRA_PORTRAITS)

$(BUILD)/obj/krillin_sprites_data.o: $(BUILD)/gen/krillin_sprites_data.c src/coop/rom.h
	@mkdir -p $(BUILD)/obj
	$(CC) $(CFLAGS) -Isrc/coop -c $< -o $@

$(BUILD)/obj/a16_sprites_data.o: $(BUILD)/gen/a16_sprites_data.c src/coop/rom.h
	@mkdir -p $(BUILD)/obj
	$(CC) $(CFLAGS) -Isrc/coop -c $< -o $@

# Android 13 and Cooler's Namekian servant (never committed); previews beside them
$(BUILD)/gen/boss_models_data.c: tools/bossmodels.py tools/a16sprites.py tools/buu2log2.py tools/sprites.py tools/mapchunks.py $(ROM)
	$(PYTHON) tools/bossmodels.py $(ROM) $@ --preview-dir $(BUILD)

$(BUILD)/obj/boss_models_data.o: $(BUILD)/gen/boss_models_data.c src/coop/rom.h
	@mkdir -p $(BUILD)/obj
	$(CC) $(CFLAGS) -Isrc/coop -c $< -o $@

# Paragus (sprite + dialogue portrait) and the Saiyan ship for the Broly chapter (never committed); preview beside them
$(BUILD)/gen/broly_sprites_data.c: tools/brolysprites.py tools/a16sprites.py tools/buu2log2.py tools/buu_sprites.py tools/sprites.py tools/mapchunks.py $(ROM) $(if $(filter 1,$(BUU)),$(BUU_ROM))
	$(PYTHON) tools/brolysprites.py $(ROM) $@ --buu $(if $(filter 1,$(BUU)),$(BUU_ROM),none) --preview-dir $(BUILD)

$(BUILD)/obj/broly_sprites_data.o: $(BUILD)/gen/broly_sprites_data.c src/coop/rom.h
	@mkdir -p $(BUILD)/obj
	$(CC) $(CFLAGS) -Isrc/coop -c $< -o $@

# Hercule's record 72 with the Kamehameha pose (group 18) for the Cell Games beam struggle (never committed);
# preview beside it (Goku and Gohan's beam frames above Hercule's)
$(BUILD)/gen/hercule_sprites_data.c: tools/herculesprites.py tools/a16sprites.py tools/buu2log2.py tools/sprites.py tools/mapchunks.py $(ROM)
	$(PYTHON) tools/herculesprites.py $(ROM) $@ --preview-dir $(BUILD)

$(BUILD)/obj/hercule_sprites_data.o: $(BUILD)/gen/hercule_sprites_data.c src/coop/rom.h
	@mkdir -p $(BUILD)/obj
	$(CC) $(CFLAGS) -Isrc/coop -c $< -o $@

# Goku and Super Saiyan Goku with a halo on every frame, for display rows 26 / 27 (never committed); preview beside them
$(BUILD)/gen/halo_sprites_data.c: tools/halosprites.py tools/brolysprites.py tools/a16sprites.py tools/buu2log2.py tools/sprites.py tools/mapchunks.py $(ROM)
	$(PYTHON) tools/halosprites.py $(ROM) $@ --preview-dir $(BUILD)

$(BUILD)/obj/halo_sprites_data.o: $(BUILD)/gen/halo_sprites_data.c src/coop/rom.h
	@mkdir -p $(BUILD)/obj
	$(CC) $(CFLAGS) -Isrc/coop -c $< -o $@

# Super Saiyan 2 Gohan's record 58 with the hurt / lying / tumble groups from record 59 (party slot 8, display
# row 28; never committed); preview beside it
$(BUILD)/gen/ssj2_sprites_data.c: tools/ssj2sprites.py tools/sprites.py tools/mapchunks.py $(ROM)
	$(PYTHON) tools/ssj2sprites.py $(ROM) $@ --preview-dir $(BUILD)

$(BUILD)/obj/ssj2_sprites_data.o: $(BUILD)/gen/ssj2_sprites_data.c src/coop/rom.h

# Super Android 13 (sprite, ki orbs, dialogue portrait) for the post-game (never committed); preview beside it
$(BUILD)/gen/super13_sprites_data.c: tools/super13sprites.py tools/buu2log2.py tools/buu_sprites.py tools/sprites.py tools/mapchunks.py $(ROM) $(if $(filter 1,$(BUU)),$(BUU_ROM))
	$(PYTHON) tools/super13sprites.py $(ROM) $@ --buu $(if $(filter 1,$(BUU)),$(BUU_ROM),none) --preview-dir $(BUILD)

$(BUILD)/obj/super13_sprites_data.o: $(BUILD)/gen/super13_sprites_data.c src/coop/rom.h
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

$(BUILD)/maps.patch.json: $(MAP_SPECS) tools/mapbuild.py tools/maplib.py tools/mapchunks.py tools/buu2log2.py tools/script_asm.py $(wildcard data/story/*.lscript) $(ROM) $(if $(filter 1,$(BUU)),$(BUU_ROM))
	@mkdir -p $(BUILD)
	$(PYTHON) tools/mapbuild.py build $(MAP_SPECS) $@ --rom $(ROM) --free $(MAP_FREE) --preview $(BUILD)/maps --buu-rom $(if $(filter 1,$(BUU)),$(BUU_ROM),none)

$(OUT): $(BUILD)/coop.bin $(BUILD)/coop.sym tools/patch_rom.py $(DATA_PATCH) $(MAPS_PATCH)
	$(PYTHON) tools/patch_rom.py $(ROM) $(BUILD)/coop.bin $(BUILD)/coop.sym $@ $(PATCH_FLAGS)

clean:
	rm -rf $(BUILD)/obj $(BUILD)/coop.* $(BUILD)/hercule.patch.json $(BUILD)/maps.patch.json $(OUT)

.PHONY: all clean FORCE
