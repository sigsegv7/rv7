.SILENT:
override PROMPT := printf "%s\t\t%s\n"

CFILES = $(shell find . -name "*.c")
CFILES += $(shell find ../lib -name "*.c")
CFILES += $(shell find ../os -name "*.c")
CFILES += $(shell find ../vm -name "*.c")
CFILES += $(shell find ../dev -name "*.c")
CFILES += $(shell find ../mu -name "*.c")
OFILES = $(CFILES:.c=.o)
DFILES = $(CFILES:.c=.d)
CC =
SYS_CFLAGS =

.PHONY: all
all: $(OFILES)

-include $(DFILES)
%.o: %.c
	$(PROMPT) " [CC] " $<
	$(CC) -c $< $(SYS_CFLAGS) -MMD -I../target/inc/ -I../inc/ -I../inc/lib -o $@
