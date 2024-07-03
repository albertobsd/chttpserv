CC = gcc
CFLAGS = -Wno-write-strings -Wno-format-security -pthread -ggdb 

default: serv

serv: serv.c
	clear
	$(CC) $(CFLAGS) -o serv serv.c

fuzzer: fuzzer.c
	clear
	$(CC) $(CFLAGS) -o fuzzer fuzzer.c

clean:
	clear
	rm serv
