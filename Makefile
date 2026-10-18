ASM = nasm
CC = clang
LD = ld

# Обрати внимание на --target=i386-elf
CFLAGS = --target=i386-elf -m32 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-avx -fno-builtin -O0 -c

all: darkos.img

entry.o: entry.asm
	$(ASM) -f elf32 entry.asm -o entry.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

kernel.bin: entry.o kernel.o linker.ld
	$(LD) -m elf_i386 -T linker.ld --oformat binary entry.o kernel.o -o kernel.bin

boot.bin: bootloader.asm
	$(ASM) -f bin bootloader.asm -o boot.bin

darkos.img: boot.bin kernel.bin
	cat boot.bin kernel.bin > temp.bin
	dd if=/dev/zero of=darkos.img bs=1 count=1474560 2>/dev/null
	dd if=temp.bin of=darkos.img conv=notrunc 2>/dev/null
	rm temp.bin

clean:
	rm -f *.o *.bin *.img
