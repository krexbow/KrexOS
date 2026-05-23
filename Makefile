ASM = nasm
CC = gcc
LD = ld
OBJCOPY = objcopy
QEMU = qemu-system-i386

CFLAGS = -m32 -ffreestanding -fno-pie -nostdlib -fno-stack-protector -c

# По умолчанию запускаем всё
all: os-image.bin hd.img run

# Компиляция объектов
kernel.o: src/kernel.c src/ata.h src/fs.h src/idt.h src/libc.h src/screen.h src/keyboard.h src/shell.h src/auth.h src/ui.h
	$(CC) $(CFLAGS) src/kernel.c -o kernel.o

shell.o: src/shell.c src/shell.h src/screen.h src/keyboard.h src/libc.h src/fs.h src/ata.h
	$(CC) $(CFLAGS) src/shell.c -o shell.o

auth.o: src/auth.c src/auth.h src/ui.h src/keyboard.h src/libc.h
	$(CC) $(CFLAGS) src/auth.c -o auth.o

ui.o: src/ui.c src/ui.h src/screen.h src/keyboard.h
	$(CC) $(CFLAGS) src/ui.c -o ui.o

ata.o: src/ata.c src/ata.h
	$(CC) $(CFLAGS) src/ata.c -o ata.o

fs.o: src/fs.c src/fs.h
	$(CC) $(CFLAGS) src/fs.c -o fs.o

idt.o: src/idt.c src/idt.h
	$(CC) $(CFLAGS) src/idt.c -o idt.o

libc.o: src/libc.c src/libc.h
	$(CC) $(CFLAGS) src/libc.c -o libc.o

# Добавили компиляцию экрана и клавиатуры, которых не хватало линкеру
screen.o: src/screen.c src/screen.h
	$(CC) $(CFLAGS) src/screen.c -o screen.o

keyboard.o: src/keyboard.c src/keyboard.h src/libc.h
	$(CC) $(CFLAGS) src/keyboard.c -o keyboard.o

idt_asm.o: src/idt_asm.asm
	$(ASM) -f win32 src/idt_asm.asm -o idt_asm.o

kernel_entry.o: src/kernel_entry.asm
	$(ASM) -f win32 src/kernel_entry.asm -o kernel_entry.o

# Линковка (Сюда тоже добавлены screen.o и keyboard.o)
kernel.bin: kernel_entry.o kernel.o shell.o auth.o ui.o ata.o fs.o idt.o idt_asm.o libc.o screen.o keyboard.o
	$(LD) -m i386pe -nostdlib --image-base 0 -Ttext 0x1000 -e _start \
	kernel_entry.o kernel.o shell.o auth.o ui.o ata.o fs.o idt.o idt_asm.o libc.o screen.o keyboard.o -o kernel.tmp
	$(OBJCOPY) -O binary kernel.tmp kernel.bin
	cmd /c del kernel.tmp

# Создание образа для загрузки
os-image.bin: kernel.bin src/boot.asm
	$(ASM) -f bin src/boot.asm -o os-image.bin

# Создание пустого диска
hd.img:
	cmd /c if not exist hd.img fsutil file createnew hd.img 67108864

# Запуск
run: os-image.bin hd.img
	$(QEMU) -drive file=os-image.bin,format=raw,index=0,media=disk \
	-drive file=hd.img,format=raw,index=1,media=disk \
	-boot c -no-reboot -d int -D log.txt

clean:
	cmd /c del /f /q *.bin *.o *.tmp 2>nul || exit 0