CC      = i686-elf-gcc
AS      = i686-elf-as
CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -Wextra

all: myos.bin

boot.o: boot.s
	$(AS) boot.s -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

myos.bin: boot.o kernel.o linker.ld
	$(CC) -T linker.ld -o myos.bin -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc

# Run in QEMU directly — no disk image needed, QEMU speaks Multiboot natively
run: myos.bin
	qemu-system-i386 -kernel myos.bin

clean:
	rm -f *.o myos.bin

.PHONY: all run clean
