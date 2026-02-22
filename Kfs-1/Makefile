CP := cp
RM := rm -rf
MKDIR := mkdir -pv

BIN = kernel
CFG = grub.cfg
ISO_PATH := iso
BOOT_PATH := $(ISO_PATH)/boot
GRUB_PATH := $(BOOT_PATH)/grub

CFLAGS = -m32 -c \
         -fno-builtin \
         -fno-stack-protector \
         -fno-pie \
		 -fno-exceptions\
         -nostdlib \
         -nodefaultlibs \
         -Wall \
         -Wextra \
# 		 -fno-rtti 



.PHONY: all
all: bootloader kernel.o printf.o linker iso
	@echo Make has completed

bootloader: boot.asm
	nasm -f elf32 boot.asm -o boot.o

kernel.o: kernel.c
	gcc $(CFLAGS) kernel.c -o kernel.o

printf.o: printf.c
	gcc $(CFLAGS) printf.c -o printf.o

linker: linker.ld boot.o kernel.o printf.o
	ld -m elf_i386 -T linker.ld -o $(BIN) boot.o kernel.o printf.o -nostdlib

iso: linker
	$(MKDIR) $(GRUB_PATH)
	$(CP) $(BIN) $(BOOT_PATH)
	$(CP) $(CFG) $(GRUB_PATH)
	grub-file --is-x86-multiboot $(BOOT_PATH)/$(BIN)
	grub-mkrescue -o my-kernel.iso $(ISO_PATH)

.PHONY: run
run: linker
	qemu-system-i386 -kernel $(BIN)

.PHONY: clean
clean:
	$(RM) *.o $(BIN) *.iso $(ISO_PATH)