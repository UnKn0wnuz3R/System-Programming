cc = gcc
cflags=-g -Wall
target = testlib
objects = bitmap.o debug.o hash.o hex_dump.o list.o main.o

$(target) : $(objects)
	$(cc) $(cflags) -o $@ $(objects) 


main.o: main.c bitmap.h debug.h hash.h hex_dump.h limits.h list.h round.h
	$(CC) $(CFLAGS) -c main.c

bitmap.o: bitmap.c bitmap.h hex_dump.h limits.h round.h
	$(CC) $(CFLAGS) -c bitmap.c

debug.o: debug.c debug.h
	$(CC) $(CFLAGS) -c debug.c

hash.o: hash.c hash.h
	$(CC) $(CFLAGS) -c hash.c

hex_dump.o: hex_dump.c hex_dump.h round.h
	$(CC) $(CFLAGS) -c hex_dump.c

list.o: list.c list.h
	$(CC) $(CFLAGS) -c list.c

.PHONY : clean
clean :
		rm $(target) $(objects)
