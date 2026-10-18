ASM = nasm
CC = clang
LD = ld

# Флаги для компиляции C-ядра
CFLAGS = --target=i386-elf -m32 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-avx -fno-builtin -O0 -c

all: darkos_hdd.img

# 1. Сборка 16-битного загрузчика
boot.bin: boot/bootloader.asm
	$(ASM) -f bin boot/bootloader.asm -o boot.bin

# 2. Сборка 32-битной точки входа (вызывает main)
entry.o: kernel/entry.asm
	$(ASM) -f elf32 kernel/entry.asm -o entry.o

# 3. Компиляция ядра на C
kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) kernel/kernel.c -o kernel.o

# 4. Линковка ядра
kernel.bin: entry.o kernel.o kernel/linker.ld
	$(LD) -m elf_i386 -T kernel/linker.ld --oformat binary entry.o kernel.o -o kernel.bin

# 5. Склейка загрузчика и ядра в один плоский образ
darkos.img: boot.bin kernel.bin
	cat boot.bin kernel.bin > temp.bin
	dd if=/dev/zero of=darkos.img bs=1 count=1474560 2>/dev/null
	dd if=temp.bin of=darkos.img conv=notrunc 2>/dev/null
	rm temp.bin

# 6. Создание образа жесткого диска на 10 МБ (для Vectras VM)
darkos_hdd.img: darkos.img
	dd if=/dev/zero of=darkos_hdd.img bs=1M count=10
	dd if=darkos.img of=darkos_hdd.img conv=notrunc 2>/dev/null

clean:
	rm -f *.o *.bin *.img
