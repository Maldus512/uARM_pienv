ARMGNU ?= aarch64-elf

CFLAGS := -Wall -pedantic -ffreestanding -nostdlib -nostartfiles
ifdef APP
	CFLAGS += -DAPP
endif


# The intermediate directory for compiled object files.
BUILD = build/
# The directory in which source files are stored.
SOURCE = source/
# The directory in which header files are stored.
INCLUDE = include/
# The name of the output file to generate.
TARGET = kernel8.img

# The name of the assembler listing file to generate.
LIST = kernel.list

# The name of the map file to generate.
MAP = kernel.map

# The name of the linker script to use.
LINKER = kernel.ld

# The names of all object files that must be generated. Deduced from the 
# assembly code files in source.
OBJECTS := $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(wildcard $(SOURCE)*.c))
OBJECTS += $(patsubst $(SOURCE)%.S,$(BUILD)%.o,$(wildcard $(SOURCE)*.S))


# Rule to make everything.
all: $(TARGET) $(LIST)

# Rule to remake everything. Does not include clean.
rebuild: clean all

# Rule to make the listing file.
$(LIST) : $(BUILD)output.elf
	$(ARMGNU)-objdump -d $(BUILD)output.elf > $(LIST)

# Rule to make the image file.
$(TARGET) : $(BUILD)output.elf
	$(ARMGNU)-objcopy $(BUILD)output.elf -O binary $(TARGET)
	cp $(TARGET) boot/

# Rule to make the elf file.
$(BUILD)output.elf : $(OBJECTS) $(LINKER) $(APP)
	$(ARMGNU)-ld -nostdlib -nostartfiles $(OBJECTS) $(APP) -Map $(MAP) -T $(LINKER) -o $(BUILD)output.elf

$(BUILD)%.o: $(SOURCE)%.S
	$(ARMGNU)-gcc $(CFLAGS) -c -I $(INCLUDE) -g $< -o $@

$(BUILD)%.o: $(SOURCE)%.c
	$(ARMGNU)-gcc $(CFLAGS) -c -I $(INCLUDE) -g $< -o $@

run: all
	qemu-system-aarch64 -M raspi3 -kernel $(TARGET) -serial stdio

run2: all
	qemu-system-aarch64 -M raspi3 -kernel $(TARGET) -serial null -serial stdio

debug: all
	qemu-system-aarch64 -M raspi3 -kernel $(TARGET) -serial stdio -s -S

# Rule to clean files.
clean : 
	-rm -rf $(BUILD)*
	-rm -f $(TARGET)
	-rm -f $(LIST)
	-rm -f $(MAP)
	-rm -f *.img

.PHONY: all rebuild clean run