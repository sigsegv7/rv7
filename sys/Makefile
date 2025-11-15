CC =
LD =
SYS_CFLAGS =
ARCH =

.PHONY: all
all: arch

.PHONY: arch
arch:
	cd arch/$(ARCH); make CC=$(CC) LD=$(LD) SYS_CFLAGS="$(SYS_CFLAGS)"

.PHONY: clean
clean:
	cd arch/$(ARCH); make clean
