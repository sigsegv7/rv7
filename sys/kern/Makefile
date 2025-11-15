CFILES = $(shell find . -name "*.c")
OFILES = $(CFILES:.c=.o)
CC =
SYS_CFLAGS =

.PHONY: all
all: $(OFILES)

%.o: %.c
	$(CC) -c $< $(SYS_CFLAGS) -I../inc/ -o $@
