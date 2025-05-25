# An admittedly primitive Makefile
# To compile, type "make" or make "all"
# To remove files, type "make clean"

CC = gcc
CFLAGS = -Wall
OBJS = wserver.o wclient.o request.o io_helper.o sql.o helper.o helper_testing.o

.SUFFIXES: .c .o 

all: wserver wclient spin.cgi sql.cgi

wserver: wserver.o request.o io_helper.o helper_testing.o helper.o
	$(CC) $(CFLAGS) -o wserver wserver.o request.o io_helper.o helper_testing.o helper.o

wclient: wclient.o io_helper.o
	$(CC) $(CFLAGS) -o wclient wclient.o io_helper.o

spin.cgi: spin.c
	$(CC) $(CFLAGS) -o spin.cgi spin.c


sql.cgi: sql.o helper.o helper_testing.o request.o io_helper.o
	$(CC) $(CFLAGS) -o sql.cgi sql.o helper.o helper_testing.o request.o io_helper.o

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	-rm -f $(OBJS) wserver wclient spin.cgi
	-rm -f $(OBJS) wserver wclient sql.cgi
