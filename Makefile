CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
NM      := arm-none-eabi-nm
PYTHON  := python -I

BUILD   := build
ROM     := roms/log2.gba
OUT     := $(BUILD)/log2-coop.gba

# P2_MODE: 0 = link cable (default), 1 = mirror P1 (test), 2 = Select swaps the single controller
P2_MODE ?= 0
CFLAGS  := -mthumb -mcpu=arm7tdmi -Os -Wall -Wextra -ffreestanding -fno-builtin -fomit-frame-pointer \
           -ffunction-sections -fdata-sections -std=c11 -DP2_MODE=$(P2_MODE)
LDFLAGS := -nostdlib -nostartfiles -T src/coop/coop.ld -Wl,--gc-sections -Wl,-Map,$(BUILD)/coop.map

SRCS := $(wildcard src/coop/*.c)
OBJS := $(patsubst src/coop/%.c,$(BUILD)/obj/%.o,$(SRCS))

all: $(OUT)

$(BUILD)/obj/%.o: src/coop/%.c src/coop/rom.h src/coop/coop.h
	@mkdir -p $(BUILD)/obj
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/coop.elf: $(OBJS) src/coop/coop.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -lgcc -o $@

$(BUILD)/coop.bin: $(BUILD)/coop.elf
	$(OBJCOPY) -O binary -j .text $< $@

$(BUILD)/coop.sym: $(BUILD)/coop.elf
	$(NM) $< > $@

$(OUT): $(BUILD)/coop.bin $(BUILD)/coop.sym tools/patch_rom.py
	$(PYTHON) tools/patch_rom.py $(ROM) $(BUILD)/coop.bin $(BUILD)/coop.sym $@

clean:
	rm -rf $(BUILD)/obj $(BUILD)/coop.* $(OUT)

.PHONY: all clean
