
CC=gcc

CFLAGS=-O3 -Wall -g -pg

pubmydns : interface_ip.o critbit.o readfile.o writefile.o pubmydns.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

install: pubmydns
	@mkdir -p $(HOME)/bin
	@cp $^ $(HOME)/bin

