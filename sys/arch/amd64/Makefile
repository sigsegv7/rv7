.SILENT:
override PROMPT := printf "%s\t\t%s\n"

CC =
LD =
SYS_CFLAGS =
ASMFILES = $(shell find . -name "*.S")
CFILES = $(shell find . -name "*.c")
ASMOBJS = $(ASMFILES:.S=.S.o)
COBJ = $(CFILES:.c=.o)
MISC_OFILES = $(shell find ../../ -name "*.o")

.PHONY: all
all: $(ASMOBJS) $(COBJ)
	$(LD) -Tconf/link.ld $(MISC_OFILES) -o ../../rv7

%.S.o: %.S
	$(PROMPT) " [AS] " $<
	$(CC) -c -I../../target/inc/ $(SYS_CFLAGS) $< -o $@

%.o: %.c
	$(PROMPT) " [CC] " $<
	$(CC) -c -I../../target/inc/ -I../../inc/ $(SYS_CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm $(MISC_OFILES)
