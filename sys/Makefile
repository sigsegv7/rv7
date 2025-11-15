.SILENT:
override PROMPT := printf "%s\t\t%s\n"

CC =
LD =
SYS_CFLAGS =
ARCH =

.PHONY: all
all: kern arch

.PHONY: arch
arch:
	$(PROMPT) " [ARCH] "
	cd arch/$(ARCH); make CC=$(CC) LD=$(LD) SYS_CFLAGS="$(SYS_CFLAGS)"

.PHONY: kern
kern:
	$(PROMPT) " [KERN] "
	cd kern/; make CC=$(CC) LD=$(LD) SYS_CFLAGS="$(SYS_CFLAGS)"

.PHONY: clean
clean:
	cd arch/$(ARCH); make clean
