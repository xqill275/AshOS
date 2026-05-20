CC      = i686-elf-gcc
AS      = i686-elf-as
CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -Wextra

# Directories
BOOT_DIR   = boot
KERNEL_DIR = kernel
BUILD_DIR  = build

# Output binary
TARGET = $(BUILD_DIR)/myos.bin

# Object files
OBJS = $(BUILD_DIR)/boot.o \
       $(BUILD_DIR)/kernel.o \
       $(BUILD_DIR)/gdt.o \
       $(BUILD_DIR)/gdt_asm.o

all: $(BUILD_DIR) $(TARGET)

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/boot.o: $(BOOT_DIR)/boot.s
	$(AS) $(BOOT_DIR)/boot.s -o $(BUILD_DIR)/boot.o

$(BUILD_DIR)/kernel.o: $(KERNEL_DIR)/kernel.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/kernel.c -o $(BUILD_DIR)/kernel.o

$(BUILD_DIR)/gdt.o: $(KERNEL_DIR)/gdt.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/gdt.c -o $(BUILD_DIR)/gdt.o

$(BUILD_DIR)/gdt_asm.o: $(KERNEL_DIR)/gdt.s
	$(AS) $(KERNEL_DIR)/gdt.s -o $(BUILD_DIR)/gdt_asm.o

$(TARGET): $(OBJS) linker.ld
	$(CC) -T linker.ld -o $(TARGET) -ffreestanding -O2 -nostdlib $(OBJS) -lgcc

run: $(TARGET)
	qemu-system-i386 -kernel $(TARGET)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean
