.SILENT:
override PROMPT := printf "%s\t\t%s\n"

CC =
LD =
SYS_CFLAGS =
ARCH =

.PHONY: all
all: target kern arch
	rm -rf target/

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

.PHONY: target
target:
	mkdir -p target/inc/md/
	rsync -avr inc/arch/$(ARCH)/* target/inc/md/
