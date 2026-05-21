CC     = i686-elf-gcc
AS     = i686-elf-as
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra

BOOT_DIR   = boot
KERNEL_DIR = kernel
DRIVERS_DIR = drivers
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
       $(BUILD_DIR)/kprintf.o \
       $(BUILD_DIR)/pmm.o \
       $(BUILD_DIR)/vmm.o \
       $(BUILD_DIR)/heap.o \
       $(BUILD_DIR)/process.o \
       $(BUILD_DIR)/process_asm.o \
       $(BUILD_DIR)/ata.o \
       $(BUILD_DIR)/fs.o

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

$(BUILD_DIR)/keyboard.o: $(DRIVERS_DIR)/keyboard.c
	$(CC) $(CFLAGS) -c $(DRIVERS_DIR)/keyboard.c -o $(BUILD_DIR)/keyboard.o

$(BUILD_DIR)/shell.o: $(KERNEL_DIR)/shell.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/shell.c -o $(BUILD_DIR)/shell.o

$(BUILD_DIR)/kprintf.o: $(KERNEL_DIR)/kprintf.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/kprintf.c -o $(BUILD_DIR)/kprintf.o

$(BUILD_DIR)/pmm.o: $(KERNEL_DIR)/pmm.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/pmm.c -o $(BUILD_DIR)/pmm.o

$(BUILD_DIR)/vmm.o: $(KERNEL_DIR)/vmm.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/vmm.c -o $(BUILD_DIR)/vmm.o

$(BUILD_DIR)/heap.o: $(KERNEL_DIR)/heap.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/heap.c -o $(BUILD_DIR)/heap.o

$(BUILD_DIR)/process.o: $(KERNEL_DIR)/process.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/process.c -o $(BUILD_DIR)/process.o

$(BUILD_DIR)/process_asm.o: $(KERNEL_DIR)/process.s
	$(AS) $(KERNEL_DIR)/process.s -o $(BUILD_DIR)/process_asm.o

$(BUILD_DIR)/ata.o: $(DRIVERS_DIR)/ata.c
	$(CC) $(CFLAGS) -c $(DRIVERS_DIR)/ata.c -o $(BUILD_DIR)/ata.o

$(BUILD_DIR)/fs.o: $(KERNEL_DIR)/fs.c
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/fs.c -o $(BUILD_DIR)/fs.o

$(TARGET): $(OBJS) linker.ld
	$(CC) -T linker.ld -o $(TARGET) -ffreestanding -O2 -nostdlib $(OBJS) -lgcc

run: $(TARGET)
	qemu-system-i386 -kernel $(TARGET) -drive file=disk.img,format=raw,index=0,media=disk

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean
