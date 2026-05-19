ASM = nasm
CC = gcc
LD = ld
OBJCOPY = objcopy
QEMU = qemu-system-i386

CFLAGS = -m32 -ffreestanding -fno-pie -nostdlib -fno-stack-protector -c

all: clean os-image.bin hd.img run

# Компилируем главное ядро
kernel.o: src/kernel.c
	$(CC) $(CFLAGS) src/kernel.c -o kernel.o

# Компилируем модуль диска ATA
ata.o: src/ata.c src/ata.h
	$(CC) $(CFLAGS) src/ata.c -o ata.o

# Компилируем ассемблерную точку входа
kernel_entry.o: src/kernel_entry.asm
	$(ASM) -f win32 src/kernel_entry.asm -o kernel_entry.o

# Линкуем всё вместе БЕЗ ВАРНИНГОВ. 
# Под Win32 MinGW линкер ТРЕБУЕТ два подчеркивания: __start
kernel.bin: kernel_entry.o kernel.o ata.o
	$(LD) -m i386pe -nostdlib --image-base 0 -Ttext 0x1000 -e start kernel_entry.o kernel.o ata.o -o kernel.tmp
	$(OBJCOPY) -O binary kernel.tmp kernel.bin
	cmd /c del kernel.tmp

# Финальный образ диска
os-image.bin: kernel.bin src/boot.asm
	$(ASM) -f bin src/boot.asm -o os-image.bin

# Создание пустого диска на 64 МБ через стандартный fsutil Windows
hd.img:
	cmd /c if not exist hd.img fsutil file createnew hd.img 67108864

# Запуск: Твой os-image.bin теперь ЖЕСТКИЙ ДИСК №0, а hd.img - ЖЕСТКИЙ ДИСК №1
# Загрузка идет с жесткого диска (-boot c), QEMU больше не будет дропать чтение ядра!
run: os-image.bin hd.img
	$(QEMU) -drive file=os-image.bin,format=raw,index=0,media=disk -drive file=hd.img,format=raw,index=1,media=disk -boot c -no-reboot

clean:
	cmd /c del /f /q *.bin *.o *.tmp 2>nul || exit 0