.POSIX:

CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS =
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

all: dict-sl

dict-sl: dict.c config.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

config.h: config.def.h
	cp $< $@

clean:
	rm -f dict-sl config.h

install: all
	mkdir -p "$(DESTDIR)$(PREFIX)/bin"
	cp -f dict-sl "$(DESTDIR)$(PREFIX)/bin"
	chmod 755 "$(DESTDIR)$(PREFIX)/bin/dict-sl"
	mkdir -p "$(DESTDIR)$(MANPREFIX)"
	mkdir -p "$(DESTDIR)$(MANPREFIX)/man1"
	cp -f dict-sl.1 "$(DESTDIR)$(MANPREFIX)/man1"
	chmod 644 "$(DESTDIR)$(MANPREFIX)/man1/dict-sl.1"
	mkdir -p "$(DESTDIR)$(PREFIX)/share/dict-sl"
	cp -r vendor/* "$(DESTDIR)$(PREFIX)/share/dict-sl"
	cp -r scripts/* "$(DESTDIR)$(PREFIX)/share/dict-sl"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/bin/dict-sl"
	rm -f "$(DESTDIR)$(MANPREFIX)/man1/dict-sl.1"
	rm -rf "$(DESTDIR)$(PREFIX)/share/dict-sl"

.PHONY: all clean
