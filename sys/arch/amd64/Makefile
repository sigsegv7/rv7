.SILENT:
override PROMPT := printf "%s\t\t%s\n"

CC =
LD =
SYS_CFLAGS =
ASMFILES = $(shell find . -name "*.S" | grep -v "boot/")
CFILES = $(shell find . -name "*.c")
ASMOBJS = $(ASMFILES:.S=.S.o)
COBJ = $(CFILES:.c=.o)
DFILES = $(CFILES:.c=.d)
MISC_OFILES = $(shell find ../../ -name "*.o" | grep -v "boot/")

.PHONY: all
all: boot $(ASMOBJS) $(COBJ)
	$(LD) -Tconf/link.ld $(MISC_OFILES) -o ../../rv7
	objcopy --update-section .trampoline=boot/apboot.bin ../../rv7

%.S.o: %.S
	$(PROMPT) " [AS] " $<
	$(CC) -c -I../../target/inc/ $(SYS_CFLAGS) $< -o $@

-include $(DFILES)
%.o: %.c
	$(PROMPT) " [CC] " $<
	$(CC) -c -MMD -I../../target/inc/ -I../../inc/ $(SYS_CFLAGS) $< -o $@

.PHONY: boot
boot:
	nasm -fbin boot/apboot.asm -o boot/apboot.bin

.PHONY: clean
clean:
	rm $(MISC_OFILES)
