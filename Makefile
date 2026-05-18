ASM = nasm
CC = gcc
LD = ld
OBJCOPY = objcopy
QEMU = qemu-system-i386

CFLAGS = -m32 -ffreestanding -fno-pie -nostdlib -fno-stack-protector -c

all: run

# Компилируем Си
kernel.o: src/kernel.c
	$(CC) $(CFLAGS) src/kernel.c -o kernel.o

# Компилируем ассемблерную точку входа для ядра
kernel_entry.o: src/kernel_entry.asm
	$(ASM) -f win32 src/kernel_entry.asm -o kernel_entry.o

# Линкуем их вместе: kernel_entry.o ОБЯЗАН идти первым!
kernel.bin: kernel.o kernel_entry.o
	$(LD) -m i386pe -nostdlib --image-base 0 -Ttext 0x1000 -e _start kernel_entry.o kernel.o -o kernel.tmp
	$(OBJCOPY) -O binary kernel.tmp kernel.bin
	cmd /c del kernel.tmp

# Финальный образ диска
os-image.bin: kernel.bin src/boot.asm
	$(ASM) -f bin src/boot.asm -o os-image.bin

run: os-image.bin
	$(QEMU) -fda os-image.bin -boot a -no-reboot

clean:
	cmd /c del /f /q *.bin *.o *.tmp 2>nul || exit 0

