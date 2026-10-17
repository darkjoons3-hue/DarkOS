ASM = nasm
IMG = darkos.img
SRC = bootloader.asm

all: $(IMG)

$(IMG): $(SRC)
	$(ASM) -f bin $(SRC) -o $(IMG)

clean:
	rm -f $(IMG)
