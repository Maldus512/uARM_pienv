ARMGNU ?= aarch64-elf

#CFLAGS := -Wall -pedantic -ffreestanding -nostdlib -nostartfiles
CFLAGS := -Wall -ffreestanding -nostdlib -nostartfiles -O0
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
LINKER = hal.ld

# The names of all object files that must be generated. Deduced from the 
# assembly code files in source.
OBJECTS := $(BUILD)font.o
OBJECTS += $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(wildcard $(SOURCE)*.c))
OBJECTS += $(patsubst $(SOURCE)%.S,$(BUILD)%.o,$(wildcard $(SOURCE)*.S))
HEADERS := $(wildcard $(INCLUDE)*.h)


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
	cp $(TARGET) ../boot/

# Rule to make the elf file.
$(BUILD)output.elf : $(OBJECTS) $(LINKER) $(APP)
	$(ARMGNU)-ld -nostdlib -nostartfiles $(OBJECTS) $(APP) -Map $(MAP) -T $(LINKER) -o $(BUILD)output.elf

$(BUILD)font.o: font.psf
	$(ARMGNU)-ld -r -b binary -o $(BUILD)font.o font.psf

$(BUILD)%.o: $(SOURCE)%.S
	$(ARMGNU)-gcc $(CFLAGS) -c -I $(INCLUDE) -g $< -o $@

$(BUILD)%.o: $(SOURCE)%.c $(HEADERS)
	$(ARMGNU)-gcc $(CFLAGS) -c -I $(INCLUDE) -g $< -o $@

run: all
# Terminate ographic with ctrl-A + X
	cpulimit -l 50 qemu-system-aarch64 -M raspi3 -kernel $(TARGET) -drive file=test.dd,if=sd,format=raw -nographic

run2: all
	cpulimit -l 50 qemu-system-aarch64 -M raspi3 -kernel $(TARGET) -drive file=test.dd,if=sd,format=raw -serial stdio

debug: all
	qemu-system-aarch64 -M raspi3 -kernel $(TARGET) -drive file=test.dd,if=sd,format=raw -serial stdio -s -S

# Rule to clean files.
clean : 
	-rm -rf *.elf
	-rm -rf $(BUILD)*
	-rm -f $(TARGET)
	-rm -f $(LIST)
	-rm -f $(MAP)
	-rm -f *.img

.PHONY: all rebuild clean run