# Toolchain
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

# Target
TARGET = app

# Directories
SRC_DIR = src
BUILD_DIR = build

# Files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
# SRCS = $(SRC_DIR)/main.c
# OBJS = $(BUILD_DIR)/main.o

# Flags
CFLAGS  = -mcpu=cortex-m3 -mthumb -g -O0 -nostdlib
LDFLAGS = -T$(SRC_DIR)/linker.ld -nostartfiles

# Default target
all: $(BUILD_DIR)/$(TARGET).bin

# Compile
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

# Convert to binary
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Flash
flash: $(BUILD_DIR)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
		-c "program $< verify reset exit"

# Debug
debug: $(BUILD_DIR)/$(TARGET).elf
	gdb-multiarch $<

# Clean
clean:
	rm -rf $(BUILD_DIR)

size: $(BUILD_DIR)/$(TARGET).elf
	arm-none-eabi-size $<

disasm:
	arm-none-eabi-objdump -d $(BUILD_DIR)/$(TARGET).elf
