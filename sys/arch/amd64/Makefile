CC =
LD =
SYS_CFLAGS =
ASMFILES = $(shell find . -name "*.S")
ASMOBJS = $(ASMFILES:.S=.S.o)
MISC_OFILES = $(shell find ../../ -name "*.o")

.PHONY: all
all: $(ASMOBJS)
	$(LD) -Tconf/link.ld $(MISC_OFILES) -o ../../rv7

%.S.o: %.S
	$(CC) -c $(SYS_CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm $(MISC_OFILES)
