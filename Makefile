ARMGNU ?= arm-none-eabi

FLAGS := -mfpu=vfp -mfloat-abi=softfp -march=armv6zk -mtune=arm1176jzf-s
#TODO find out why it doesn't work without the -O2 flag (or -O)
CFLAGS := -Wall -pedantic -nostdlib -nostartfiles -ffreestanding $(FLAGS)
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
TARGET = kernel.img

# The name of the assembler listing file to generate.
LIST = kernel.list

# The name of the map file to generate.
MAP = kernel.map

# The name of the linker script to use.
LINKER = kernel.ld

# The names of all object files that must be generated. Deduced from the 
# assembly code files in source.
OBJECTS := $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(wildcard $(SOURCE)*.c))
OBJECTS += $(patsubst $(SOURCE)%.s,$(BUILD)%.o,$(wildcard $(SOURCE)*.s))

# Rule to make everything.
all: $(TARGET) $(LIST)

# Rule to remake everything. Does not include clean.
rebuild: all

# Rule to make the listing file.
$(LIST) : $(BUILD)output.elf
	$(ARMGNU)-objdump -g -d $(BUILD)output.elf > $(LIST)

# Rule to make the image file.
$(TARGET) : $(BUILD)output.elf
	$(ARMGNU)-objcopy $(BUILD)output.elf -O binary $(TARGET) 
	cp $(TARGET) boot

# Rule to make the elf file.
$(BUILD)output.elf : $(OBJECTS) $(LINKER)# $(INIT)
	$(ARMGNU)-gcc -nostartfiles $(OBJECTS) $(APP) -Wl,-Map,$(MAP),-T,$(LINKER) -o $(BUILD)output.elf

$(BUILD)%.o: $(SOURCE)%.s $(BUILD)
	$(ARMGNU)-gcc $(CFLAGS) -c -I $(INCLUDE) -g $< -o $@
	#$(ARMGNU)-as -I $(SOURCE) -g $< -o $@

# Rule to make the object files.
$(BUILD)%.o: $(SOURCE)%.c $(BUILD)
	$(ARMGNU)-gcc $(CFLAGS) -c -I $(INCLUDE) -g $< -o $@


$(BUILD):
	mkdir $@

# Rule to clean files.
clean : 
	-rm -rf $(BUILD)
	-rm -f $(TARGET)
	-rm -f $(LIST)
	-rm -f $(MAP)