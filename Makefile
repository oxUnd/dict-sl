CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS =

all: dict-sl

dict-sl: dict.c config.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

config.h: config.def.h
	cp $< $@

clean:
	rm -f dict-sl config.h

.PHONY: all clean
