.SILENT:
override PROMPT := printf "%s\t\t%s\n"

CFILES = $(shell find . -name "*.c")
CFILES += $(shell find ../lib -name "*.c")
CFILES += $(shell find ../os -name "*.c")
OFILES = $(CFILES:.c=.o)
CC =
SYS_CFLAGS =

.PHONY: all
all: $(OFILES)

%.o: %.c
	$(PROMPT) " [CC] " $<
	$(CC) -c $< $(SYS_CFLAGS) -I../inc/ -I../inc/lib -o $@
