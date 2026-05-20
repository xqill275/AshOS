CC     = i686-elf-gcc
AS     = i686-elf-as
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra

BOOT_DIR   = boot
KERNEL_DIR = kernel
BUILD_DIR  = build

TARGET = $(BUILD_DIR)/myos.bin

OBJS = $(BUILD_DIR)/boot.o \
       $(BUILD_DIR)/kernel.o \
       $(BUILD_DIR)/gdt.o \
       $(BUILD_DIR)/gdt_asm.o \
       $(BUILD_DIR)/idt.o \
       $(BUILD_DIR)/idt_asm.o \
       $(BUILD_DIR)/keyboard.o \
       $(BUILD_DIR)/shell.o \
       $(BUILD_DIR)/kprintf.o

all: $(BUILD_DIR) $(TARGET)

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

$(BUILD_DIR)/idt.o: $(KERNEL_DIR)/idt.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/idt.c -o $(BUILD_DIR)/idt.o

$(BUILD_DIR)/idt_asm.o: $(KERNEL_DIR)/idt.s
	$(AS) $(KERNEL_DIR)/idt.s -o $(BUILD_DIR)/idt_asm.o

$(BUILD_DIR)/keyboard.o: $(KERNEL_DIR)/keyboard.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/keyboard.c -o $(BUILD_DIR)/keyboard.o

$(BUILD_DIR)/shell.o: $(KERNEL_DIR)/shell.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/shell.c -o $(BUILD_DIR)/shell.o

$(BUILD_DIR)/kprintf.o: $(KERNEL_DIR)/kprintf.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/kprintf.c -o $(BUILD_DIR)/kprintf.o

$(TARGET): $(OBJS) linker.ld
	$(CC) -T linker.ld -o $(TARGET) -ffreestanding -O2 -nostdlib $(OBJS) -lgcc

run: $(TARGET)
	qemu-system-i386 -kernel $(TARGET)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean
