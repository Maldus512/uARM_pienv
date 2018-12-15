ARMGNU ?= aarch64-elf

CFLAGS := -Wall -pedantic -ffreestanding -nostdlib -nostartfiles -O0

# The intermediate directory for compiled object files.
BUILD = build/
# The directory in which source files are stored.
SOURCE = source/
# The directory in which header files are stored.
INCLUDE = include/
HALINCLUDE = ../hal/include/ 
# The name of the output file to generate.
TARGET = app.elf



# The names of all object files that must be generated. Deduced from the 
# assembly code files in source.
OBJECTS := $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(wildcard $(SOURCE)*.c))
OBJECTS += $(patsubst $(SOURCE)%.S,$(BUILD)%.o,$(wildcard $(SOURCE)*.S))



# Rule to make everything.
all: $(TARGET)

# Rule to remake everything. Does not include clean.
rebuild: clean all

# Rule to make the elf file.
$(TARGET) : $(OBJECTS)
	$(ARMGNU)-gcc -nostdlib -nostartfiles -Wl,-r -g $(OBJECTS) -o $(TARGET)

$(BUILD)%.o: $(SOURCE)%.S
	$(ARMGNU)-gcc $(CFLAGS) -c -I $(INCLUDE) -I $(HALINCLUDE)  -g $< -o $@

$(BUILD)%.o: $(SOURCE)%.c
	$(ARMGNU)-gcc $(CFLAGS) -c -I $(INCLUDE) -I $(HALINCLUDE)  -g $< -o $@

# Rule to clean files.
clean : 
	-rm -rf $(BUILD)*
	-rm -f $(TARGET)

.PHONY: all rebuild clean run