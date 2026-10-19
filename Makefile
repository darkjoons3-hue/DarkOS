ASM = nasm
CC = clang
LD = ld

CFLAGS = --target=i386-elf -m32 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-avx -mgeneral-regs-only -fno-builtin -O0 -c

all: darkos.img

boot.bin: boot/bootloader.asm
	$(ASM) -f bin boot/bootloader.asm -o boot.bin

entry.o: kernel/entry.asm
	$(ASM) -f elf32 kernel/entry.asm -o entry.o

isr.o: kernel/isr.asm
	$(ASM) -f elf32 kernel/isr.asm -o isr.o

kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) kernel/kernel.c -o kernel.o

kernel.bin: entry.o isr.o kernel.o kernel/linker.ld
	$(LD) -m elf_i386 -T kernel/linker.ld --oformat binary entry.o isr.o kernel.o -o kernel.bin

darkos.img: boot.bin kernel.bin
	cat boot.bin kernel.bin > temp.bin
	dd if=/dev/zero of=darkos.img bs=1 count=1474560 2>/dev/null
	dd if=temp.bin of=darkos.img conv=notrunc 2>/dev/null
	rm temp.bin

clean:
	rm -f *.o *.bin *.img
