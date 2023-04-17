CC=gcc
CC=g++
CFLAGS=-Wall -Wextra -pedantic -std=c99 -g
LDFLAGS=
MAIN=main
TEXT_EDI_ENTRY=main.c

$(MAIN): $(TEXT_EDI_ENTRY)
	$(CC) $(CFLAGS) $(TEXT_EDI_ENTRY) -o $@

clean:
	rm -rf $(MAIN)